
FROM debian
RUN apt update
RUN apt install -y clang make cmake
RUN apt install -y python3.11 wget
RUN apt install -y libsdl2-dev python-is-python3
COPY src /ExchatgeDesktopClient/src
COPY CMakeLists.txt /ExchatgeDesktopClient/
RUN mkdir /downloads && mkdir /ExchatgeDesktopClient/libs
COPY buildDependencies.sh /
RUN chmod +x buildDependencies.sh
RUN ./buildDependencies.sh
COPY res /ExchatgeDesktopClient/res
RUN mkdir /ExchatgeDesktopClient/build && cd /ExchatgeDesktopClient/build && cmake .. && make all
ENTRYPOINT /bin/sleep 600
