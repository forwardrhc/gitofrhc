bin=HttpdServer#可执行文件
cc=g++
LDFLAGS=-lpthread

.PHONY:all
all:$(bin) cgi

$(bin):HttpdServer.cc
	$(cc) -o $@ $^ $(LDFLAGS) -std=c++11

.PHONY:cgi
cgi:
	g++ -o Cal Cal.cc

.PHONY:clean
clean:
	rm -rf $(bin) Cal output

.PHONY:output
output:
	mkdir output
	cp $(bin) output
	cp -rf webroot output
	cp Cal output/webroot
	cp start.sh output
