Simple Native Code module intentionally slowing down IIS responses.

You can register the compiled dll via GUI:
- copy the compiled dll to %windir%\System32\inetsrv\
- open IIS management
- select your server
- click "Modules"
- click "Configure Native Modules..."
- click "Register..."
- enter "IISTarPit" and the full DLL path.

TODO:
- [x] configurable paths instead of slowing down everything
- [x] configurable delay instead of fixed 1000ms
- [ ] better (automated) installation instructions
