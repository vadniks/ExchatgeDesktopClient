
# Exchatge - a secured message exchanger (desktop client)

```
_______ _     _ _______ _     _ _______ _______  ______ _______
|______  \___/  |       |_____| |_____|    |    |  ____ |______
|______ _/   \_ |_____  |     | |     |    |    |_____| |______
```

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

Client side is written entirely in C (C11).

Build is performed via [GNU Make](https://www.gnu.org/software/make) 
or via [Ninja](https://ninja-build.org/) 
with help of [CMake](https://cmake.org) build system generator.

Client side uses the following libraries: 
* [SDL2](https://github.com/libsdl-org/SDL); 
* [SDL2Net](https://github.com/libsdl-org/SDL_net); 
* [LibSodium](https://github.com/jedisct1/libsodium); 
* [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear); 
* [SQLite3](https://sqlite.org); 
* [GNU C Library (GLIBC)](https://www.gnu.org/software/libc) - therefore project targets Linux systems. 

## The project is currently in development stage

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

For convenience, build is performed automatically via shell scripts. 
Execute the following commands to download & build dependencies, 
then, to extract the executable & it's libraries, and finally, to run 
the executable itself:
```shell
chmod +x buildDependencies.sh && ./buildDependencies.sh
mkdir build && (cd build; cmake .. && make)
chmod +x extract.sh && ./extract.shhttps://ninja-build.org/
LD_LIBRARY_PATH="$(pwd)/extracted" extracted/ExchatgeDesktopClient
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
