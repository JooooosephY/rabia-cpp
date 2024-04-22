# CMake generated Testfile for 
# Source directory: /Users/jingxi_yu/Downloads/distributed/miniraft-cpp
# Build directory: /Users/jingxi_yu/Downloads/distributed/miniraft-cpp
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_raft "/Users/jingxi_yu/Downloads/distributed/miniraft-cpp/test_raft")
set_tests_properties(test_raft PROPERTIES  ENVIRONMENT "CMOCKA_MESSAGE_OUTPUT=xml;CMOCKA_XML_FILE=test_raft.xml" _BACKTRACE_TRIPLES "/Users/jingxi_yu/Downloads/distributed/miniraft-cpp/CMakeLists.txt;41;add_test;/Users/jingxi_yu/Downloads/distributed/miniraft-cpp/CMakeLists.txt;0;")
add_test(test_read_write "/Users/jingxi_yu/Downloads/distributed/miniraft-cpp/test_read_write")
set_tests_properties(test_read_write PROPERTIES  ENVIRONMENT "CMOCKA_MESSAGE_OUTPUT=xml;CMOCKA_XML_FILE=test_read_write.xml" _BACKTRACE_TRIPLES "/Users/jingxi_yu/Downloads/distributed/miniraft-cpp/CMakeLists.txt;48;add_test;/Users/jingxi_yu/Downloads/distributed/miniraft-cpp/CMakeLists.txt;0;")
subdirs("coroio")
