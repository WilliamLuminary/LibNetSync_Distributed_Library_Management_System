CXX = g++
CXXFLAGS = -std=c++17

all: serverM serverS serverL serverH client

serverM: serverM.cpp serverM.h
	$(CXX) $(CXXFLAGS) -o serverM serverM.cpp

serverS: serverS.cpp serverS.h
	$(CXX) $(CXXFLAGS) -o serverS serverS.cpp

serverL: serverL.cpp serverL.h
	$(CXX) $(CXXFLAGS) -o serverL serverL.cpp

serverH: serverH.cpp serverH.h
	$(CXX) $(CXXFLAGS) -o serverH serverH.cpp

client: client.cpp client.h
	$(CXX) $(CXXFLAGS) -o client client.cpp

clean:
	rm -f serverM serverS serverL serverH client