# MVS Projects — Status

Last updated: 2025-02-27

## Legend

- **Done** — committed, pushed to GitHub, build verified
- **Build OK** — compiles and assembles successfully
- **Pending** — not yet started

---

## Projects

| Project | GitHub | Status | Notes |
|---------|--------|--------|-------|
| **crent370** | [mvslovers/crent370](https://github.com/mvslovers/crent370) | Done | C runtime library, thread manager. config.mk has local changes (uncommitted). |
| **crent370_sdk** | [mvslovers/crent370_sdk](https://github.com/mvslovers/crent370_sdk) | Done | Header-only SDK. Used as submodule by all other projects. |
| **c2asm370** | [mvslovers/c2asm370](https://github.com/mvslovers/c2asm370) | Done | GCC cross-compiler (C → S/370 Assembly). |
| **httpd** | [mvslovers/httpd](https://github.com/mvslovers/httpd) | Done | HTTP/FTP server. Restructured (src/, include/, static/, credentials/). Build system uses hardcoded dataset names — needs migration to .env pattern. |
| **ufs370** | [mvslovers/ufs370](https://github.com/mvslovers/ufs370) | Done | Virtual file system. Build OK. .env pattern implemented. Has uncommitted build system fixes. |
| **lua370** | [mvslovers/lua370](https://github.com/mvslovers/lua370) | Done | Lua 5.4 scripting engine. Build OK (33 modules, all CC 0000/0004). |
| **mqtt370** | — | Build partially OK | Utility (20 modules) and client (45 modules) build OK. Broker (52 modules) fails: needs lua370 headers (lua.h, lualib.h, lauxlib.h). CLI (15 modules) not yet tested. Needs GitHub repo, commit, push. |
| **mvsmf** | [mvslovers/mvsmf](https://github.com/mvslovers/mvsmf) | Done | zOSMF REST API clone (CGI module for httpd). Reference project for .env pattern. |
| **ftp370** | — | Pending | FTP client. Sources in mike-orig/. |
| **ind_file** | — | Pending | IND_FILE utility. Sources in mike-orig/. |

## Pending Tasks

- [ ] Create **lua370_sdk** repo (header-only, like crent370_sdk) — needed by mqtt370/broker and httpd
- [ ] Add lua370_sdk submodule to mqtt370, fix broker build
- [ ] Create mqtt370 GitHub repo, commit, push
- [ ] Commit ufs370 build system fixes
- [ ] Migrate httpd config.mk to .env pattern
- [ ] Add mvsMF to httpd CLAUDE.md external dependencies table
- [ ] Create **ufs_sdk** repo (header-only) — needed by httpd
- [ ] Create **mqtt370_sdk** repo (header-only) — needed by httpd
- [ ] Set up ftp370 project
- [ ] Set up ind_file project
