# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# generate files with python

BUILD_DIR = ${CURDIR}/../build/

# Prerequisites on the right side of '|' is only order
all: gen_opcode gen_version gen_builtins
.PHONY: all

${BUILD_DIR}/python:
	mkdir -p $@

# generated function
GEN_FUNC_OUTPUT = ${BUILD_DIR}/python/generated_functions.py \
                 ${BUILD_DIR}/gen_cpp/opcode/functions.cc \
                 ${BUILD_DIR}/gen_cpp/opcode/functions.h
${GEN_FUNC_OUTPUT}: gen_functions.py | ${BUILD_DIR}/python
	cd ${BUILD_DIR}/python && ${PYTHON} ${CURDIR}/gen_functions.py
gen_func: ${GEN_FUNC_OUTPUT}
.PHONY: gen_func

# generated vector function
GEN_VEC_FUNC_OUTPUT = ${BUILD_DIR}/python/generated_vector_functions.py \
                 ${BUILD_DIR}/gen_cpp/opcode/vector-functions.cc \
                 ${BUILD_DIR}/gen_cpp/opcode/vector-functions.h
${GEN_VEC_FUNC_OUTPUT}: gen_vector_functions.py | ${BUILD_DIR}/python
	cd ${BUILD_DIR}/python && ${PYTHON} ${CURDIR}/gen_vector_functions.py
gen_vec_func: ${GEN_VEC_FUNC_OUTPUT}
.PHONY: gen_vec_func

# generate opcode
GEN_OPCODE_OUTPUT = ${BUILD_DIR}/thrift/Opcodes.thrift \
                    ${BUILD_DIR}/gen_cpp/opcode/opcode-registry-init.cc \
                    ${BUILD_DIR}/java/org/apache/doris/opcode/FunctionRegistry.java \
                    ${BUILD_DIR}/java/org/apache/doris/opcode/FunctionOperator.java

${GEN_OPCODE_OUTPUT}: doris_functions.py ${GEN_FUNC_OUTPUT} ${GEN_VEC_FUNC_OUTPUT} | ${BUILD_DIR}/python
gen_opcode: ${GEN_OPCODE_OUTPUT}
.PHONY: gen_opcode

# generate 
GEN_BUILTINS_OUTPUT = ${BUILD_DIR}/java/org/apache/doris/builtins/ScalarBuiltins.java

${GEN_BUILTINS_OUTPUT}: doris_builtins_functions.py gen_builtins_functions.py
	cd ${BUILD_DIR}/python && ${PYTHON} ${CURDIR}/gen_builtins_functions.py
gen_builtins: ${GEN_BUILTINS_OUTPUT}
.PHONY: gen_builtins

# generate version info
VERSION_OUTPUT = ${BUILD_DIR}/java/org/apache/doris/common/Version.java

gen_version:
	${CURDIR}/gen_build_version.sh
.PHONY: gen_version


