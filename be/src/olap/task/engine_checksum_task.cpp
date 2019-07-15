// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "olap/task/engine_checksum_task.h"

#include "olap/reader.h"

namespace doris {

EngineChecksumTask::EngineChecksumTask(TTabletId tablet_id, TSchemaHash schema_hash, 
        TVersion version, TVersionHash version_hash, uint32_t* checksum)
        :_tablet_id(tablet_id),
         _schema_hash(schema_hash),
         _version(version),
         _version_hash(version_hash),
         _checksum(checksum) {

}

OLAPStatus EngineChecksumTask::execute() {
    OLAPStatus res = _compute_checksum();
    return res;
} // execute


OLAPStatus EngineChecksumTask::_compute_checksum() {
    LOG(INFO) << "begin to process compute checksum."
              << "tablet_id=" << _tablet_id
              << ", schema_hash=" << _schema_hash
              << ", version=" << _version;
    OLAPStatus res = OLAP_SUCCESS;

    if (_checksum == NULL) {
        OLAP_LOG_WARNING("invalid output parameter which is null pointer.");
        return OLAP_ERR_CE_CMD_PARAMS_ERROR;
    }

    TabletSharedPtr tablet = StorageEngine::instance()->tablet_manager()->get_tablet(_tablet_id, _schema_hash);
    if (NULL == tablet.get()) {
        OLAP_LOG_WARNING("can't find tablet. [tablet_id=%ld schema_hash=%d]",
                         _tablet_id, _schema_hash);
        return OLAP_ERR_TABLE_NOT_FOUND;
    }


    Reader reader;
    ReaderParams reader_params;
    reader_params.tablet = tablet;
    reader_params.reader_type = READER_CHECKSUM;
    reader_params.version = Version(0, _version);

    {
        ReadLock rdlock(tablet->get_header_lock_ptr());
        const RowsetSharedPtr message = tablet->rowset_with_max_version();
        if (message == NULL) {
            LOG(FATAL) << "fail to get latest version. tablet_id=" << _tablet_id;
            return OLAP_ERR_VERSION_NOT_EXIST;
        }

        if (message->end_version() == _version
                && message->version_hash() != _version_hash) {
            OLAP_LOG_WARNING("fail to check latest version hash. "
                             "[res=%d tablet_id=%ld version_hash=%ld request_version_hash=%ld]",
                             res, _tablet_id, message->version_hash(), _version_hash);
            return OLAP_ERR_CE_CMD_PARAMS_ERROR;
        }
        OLAPStatus acquire_reader_st = tablet->capture_rs_readers(reader_params.version, &reader_params.rs_readers);
        if (acquire_reader_st != OLAP_SUCCESS) {
            LOG(WARNING) << "fail to init reader. tablet=" << tablet->full_name()
                         << "res=" << acquire_reader_st;
            return acquire_reader_st;
        }
    }

    // ignore float and double type considering to precision lose
    for (size_t i = 0; i < tablet->tablet_schema().num_columns(); ++i) {
        FieldType type = tablet->tablet_schema().column(i).type();
        if (type == OLAP_FIELD_TYPE_FLOAT || type == OLAP_FIELD_TYPE_DOUBLE) {
            continue;
        }

        reader_params.return_columns.push_back(i);
    }

    res = reader.init(reader_params);
    if (res != OLAP_SUCCESS) {
        OLAP_LOG_WARNING("initiate reader fail. [res=%d]", res);
        return res;
    }

    RowCursor row;
    res = row.init(tablet->tablet_schema(), reader_params.return_columns);
    if (res != OLAP_SUCCESS) {
        OLAP_LOG_WARNING("failed to init row cursor. [res=%d]", res);
        return res;
    }
    row.allocate_memory_for_string_type(tablet->tablet_schema());

    bool eof = false;
    uint32_t row_checksum = 0;
    while (true) {
        OLAPStatus res = reader.next_row_with_aggregation(&row, &eof);
        if (res == OLAP_SUCCESS && eof) {
            VLOG(3) << "reader reads to the end.";
            break;
        } else if (res != OLAP_SUCCESS) {
            OLAP_LOG_WARNING("fail to read in reader. [res=%d]", res);
            return res;
        }

        row_checksum = row.hash_code(row_checksum);
    }

    LOG(INFO) << "success to finish compute checksum. checksum=" << row_checksum;
    *_checksum = row_checksum;
    return OLAP_SUCCESS;
}

} // doris