
# Exchatge - a secured message exchanger (desktop client)

The purpose of this project is to easily exchange messages 
via binary protocol using an encrypted communication channel 
in the realtime. Each client-to-server connection is encrypted, 
each client-to-client connection is also encrypted. All messages 
go through server, but because of presence of the second layer 
of encryption between clients, any interception of messages by 
the server or anyone else is useless. File exchange is supported.

Project is created for Linux x86_64 desktop platforms (PCs).

## `TODO`
* Maybe add unit tests,
* Optimize UI for high dpi displays.

## Dependencies

Client side is written entirely in C.

Build is performed via [GNU Make](https://www.gnu.org/software/make) 
with help of [CMake](https://cmake.org) build system generator.

Client side uses the following libraries: 
* [SDL2](https://github.com/libsdl-org/SDL), 
* [SDL2Net](https://github.com/libsdl-org/SDL_net), 
* [LibSodium](https://github.com/jedisct1/libsodium), 
* [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear),
* [SQLite3](https://sqlite.org),
* [GNU C Library (GLIBC)](https://www.gnu.org/software/libc).

## The project is currently in Beta stage

[The server](https://github.com/vadniks/ExchatgeServer)

## Concept screenshots

![A](screenshots/a.png "A")
![B](screenshots/b.png "B")
![C](screenshots/c.png "C")
![D](screenshots/d.png "D")
![E](screenshots/e.png "E")
![F](screenshots/f.png "F")
![G](screenshots/g.png "G")
![H](screenshots/h.png "H")
![I](screenshots/i.png "I")

## Build

For convenience, build is performed automatically inside a docker container. 
All you have to do is install docker and docker-compose programs, and run 
`docker-compose up --build` within the root directory of this repository. 
Docker will download necessary files, create & launch a container. While 
Docker is creating the container, bash scripts download & build project 
dependencies inside that container. After container was created, you have 
1 minute to run the script (`export.sh`), which will export all built 
libraries & the executable itself from the container to your host machine - 
inside the directory in which the export script was launched there will 
be created a directory named `exported`, which will contain all the necessary 
files. Then, you can `run` the executable using this `command` from this 
repository's root directory:
```shell
chmod +x export.sh && ./export.sh
LD_LIBRARY_PATH="$(pwd)/exported" exported/ExchatgeDesktopClient
```
After all the necessary file were exported, the container, as well as it's image 
can be removed via these command (Ctrl + C the running container first):
```shell
docker-compose down
# and the following ones can be used to do full clean up:
docker container prune
docker image prune -a
docker builder prune
```

## License

GNU GPLv3 - to keep the source code open

---

Exchatge - a secured realtime message exchanger (desktop client).
Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
