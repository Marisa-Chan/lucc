# lucc #

lucc is a libunr powered UCC implementation.

# Building #

```
1) Download/Build libunr in the same parent directory as this repo
    - https://bitbucket.org/Xaleros/libunr

2) In the root folder, run the following commands
   - cmake -DCMAKE_BUILD_TYPE=<buildtype> -DBUILD32=<On/Off> .
        - buildtype can be "Debug", "Release", or "RelWithDebInfo"
   - make
       - or "mingw32-make" for Windows
```

# Licensing #

    GNU Affero General Public License v3