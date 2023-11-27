CXX = g++
#CXXFLAGS = -std=c++17

all: serverM serverS serverL serverH client

serverM: serverM.cpp serverM.h
	#$(CXX) $(CXXFLAGS) -o serverM serverM.cpp
	$(CXX) -o serverM serverM.cpp

serverS: serverS.cpp serverS.h
	#$(CXX) $(CXXFLAGS) -o serverS serverS.cpp
	$(CXX) -o serverS serverS.cpp

serverL: serverL.cpp serverL.h
	#$(CXX) $(CXXFLAGS) -o serverL serverL.cpp
	$(CXX) -o serverL serverL.cpp

serverH: serverH.cpp serverH.h
	#$(CXX) $(CXXFLAGS) -o serverH serverH.cpp
	$(CXX) -o serverH serverH.cpp

client: client.cpp ee450_Li_Yaxing_yaxingl/client.h
	#$(CXX) $(CXXFLAGS) -o client client.cpp
	$(CXX) -o client client.cpp

clean:
	rm -f serverM serverS serverL serverH client