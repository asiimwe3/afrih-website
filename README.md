# AFRIH — Afrika Integrated Holdings (C Server)

A Pan-African investment group website served by a hand-written HTTP server in pure C.

## What this is

All 11 pages of the AFRIH website, served by a custom C HTTP server with POSIX sockets and pthreads. No frameworks, no JavaScript libraries, no HTML files — every page is generated dynamically by C code.

## Pages

1. Home (/) — Overview with youth focus
2. Investment Thesis (/thesis) — Market opportunity + youth dividend
3. Business Model (/model) — Layered capital + 9 revenue streams
4. Youth & Skills (/youth) — Training programs, skilling, job creation
5. Sectors (/sectors) — 12 sectors with youth job opportunities
6. Governance (/governance) — Oversight bodies + Youth Advisory Board
7. Impact (/impact) — Youth training outcomes + ESG framework
8. Roadmap (/roadmap) — 10-year plan with training scale-up milestones
9. FAQ (/faq) — Investor + youth program questions
10. Invest (/invest) — 9 investment vehicles including Impact & Education funds
11. Contact (/contact) — Youth program applications + all inquiries

## Youth Focus

- 9 training programs: digital skills, agribusiness, energy, construction, healthcare, finance, logistics, entrepreneurship, tourism
- 6-step pathway: Apply → Assess → Train → Apprentice → Job/Startup → Alumni
- Job creation targets: 100K+ youth trained, 50K+ jobs, 10K+ entrepreneurs by Year 10
- Youth Advisory Board in governance structure
- Impact Fund and Education Fund dedicated to youth programs
- Scholarship sponsorship from $500

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
