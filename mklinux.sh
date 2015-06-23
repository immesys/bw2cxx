g++ -std=c++11 -I boost_1_58_0/ -c bwclient.cpp
g++ -std=c++11 -I boost_1_58_0/ -DBOOST_ASIO_SEPARATE_COMPILATION=1 -c boost_1_58_0/libs/system/src/error_code.cpp -o error_code.o

#g++ -std=c++11 -I boost_1_58_0/ -c boost_1_58_0/libs/thread/src/win32/tss_pe.cpp -o win32t0.o
#g++ -std=c++11 -I boost_1_58_0/ -c boost_1_58_0/libs/thread/src/win32/thread.cpp -o win32t1.o
#g++ -std=c++11 -I boost_1_58_0/ -c boost_1_58_0/libs/thread/src/win32/tss_dll.cpp -o win32t2.o
g++ -std=c++11 -I boost_1_58_0/ -DBOOST_ASIO_SEPARATE_COMPILATION=1 -c boost_1_58_0/libs/thread/src/future.cpp -o t3.o
g++ -std=c++11 -I boost_1_58_0/ -DBOOST_ASIO_SEPARATE_COMPILATION=1 -c boost_1_58_0/libs/thread/src/tss_null.cpp -o t4.o
g++ -std=c++11 -I boost_1_58_0/ -DBOOST_ASIO_SEPARATE_COMPILATION=1 -c boost_1_58_0/libs/thread/src/pthread/once_atomic.cpp -o pt5.o
g++ -std=c++11 -I boost_1_58_0/ -DBOOST_ASIO_SEPARATE_COMPILATION=1 -c boost_1_58_0/libs/thread/src/pthread/once.cpp -o pt6.o
g++ -std=c++11 -I boost_1_58_0/ -DBOOST_ASIO_SEPARATE_COMPILATION=1 -c boost_1_58_0/libs/thread/src/pthread/thread.cpp -o pt7.o
g++ -std=c++11 -I boost_1_58_0/ -DBOOST_ASIO_SEPARATE_COMPILATION=1 -c ./boost_1_58_0/boost/asio/impl/src.cpp -o pt8.o


g++ -std=c++11 -I boost_1_58_0/ \
  bwclient.o \
  main_test.cpp \
  pt7.o \
  pt5.o \
  error_code.o \
  -lpthread
