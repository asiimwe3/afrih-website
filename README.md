# AFRIH — Afrika Integrated Holdings (C Server)

A Pan-African investment group website served by a hand-written HTTP server in pure C.

## What this is

All 11 pages of the AFRIH website, served by a custom C HTTP server with POSIX sockets and pthreads. No frameworks, no JavaScript libraries, no HTML files — every page is generated dynamically by C code.

## Pages

1. Home (/)
2. Investment Thesis (/thesis)
3. Business Model (/model)
4. Sectors (/sectors)
5. Governance (/governance)
6. Impact & ESG (/impact)
7. Roadmap (/roadmap)
8. FAQ (/faq)
9. Investor Onboarding (/invest)
10. Contact (/contact)

Plus: robots.txt, sitemap.xml, and form handlers for contact, investor inquiry, and newsletter.

## Build & Run

```bash
gcc -O2 -o afrih_server afrih_server.c -lpthread
./afrih_server
```

Open http://localhost:8080

## Docker

```bash
docker build -t afrih .
docker run -p 80:8080 afrih
```

Built by Asiimwe Derick — DeryCode Technologies for Afrika Integrated Holdings.
