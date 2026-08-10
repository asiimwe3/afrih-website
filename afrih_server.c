/*
 * ===================================================================
 * Afrika Integrated Holdings (AFRIH) — Web Server in C
 * No HTML files. No CSS files. Everything generated from C code.
 * Pure C HTTP server using POSIX sockets.
 * ===================================================================
 * Compile: gcc afrih_server.c -o afrih_server -lpthread
 * Run:     ./afrih_server
 * Open:    http://localhost:8080
 * ===================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 65536
#define MAX_ROUTES 16

/* ------------------------------------------------------------------
 * String builder — dynamically grows, used to assemble HTML pages
 * ------------------------------------------------------------------ */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} StrBuf;

void sb_init(StrBuf *sb) {
    sb->cap = 4096;
    sb->len = 0;
    sb->data = malloc(sb->cap);
    sb->data[0] = '\0';
}

void sb_append(StrBuf *sb, const char *str) {
    size_t slen = strlen(str);
    while (sb->len + slen + 1 > sb->cap) {
        sb->cap *= 2;
        sb->data = realloc(sb->data, sb->cap);
    }
    memcpy(sb->data + sb->len, str, slen);
    sb->len += slen;
    sb->data[sb->len] = '\0';
}

void sb_free(StrBuf *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

/* ------------------------------------------------------------------
 * CSS — all styling defined as a C string constant
 * ------------------------------------------------------------------ */
#define CSS_STYLES \
"*,*::before,*::after{margin:0;padding:0;box-sizing:border-box;}" \
"body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:#0a0a14;color:#e0e0e8;line-height:1.6;}" \
"a{color:#6c63ff;text-decoration:none;}" \
"a:hover{color:#8b83ff;}" \
"/* Header */" \
".header{position:fixed;top:0;left:0;right:0;z-index:100;background:rgba(10,10,20,0.95);backdrop-filter:blur(10px);border-bottom:1px solid rgba(108,99,255,0.2);padding:0 2rem;}" \
".header-inner{max-width:1200px;margin:0 auto;display:flex;align-items:center;justify-content:space-between;height:70px;}" \
".logo{font-size:1.5rem;font-weight:800;color:#fff;letter-spacing:-0.5px;}" \
".logo span{color:#6c63ff;}" \
".logo .acronym{font-size:0.85rem;color:#43e97b;margin-left:8px;font-weight:600;}" \
".nav{display:flex;gap:2rem;align-items:center;}" \
".nav a{color:#a0a0b8;font-size:0.95rem;font-weight:500;transition:color 0.2s;}" \
".nav a:hover{color:#fff;}" \
".nav a.active{color:#6c63ff;}" \
".nav-cta{background:#6c63ff;color:#fff !important;padding:8px 20px;border-radius:8px;font-weight:600;}" \
".nav-cta:hover{background:#5a52e0;}" \
"/* Hero */" \
".hero{min-height:100vh;display:flex;align-items:center;justify-content:center;text-align:center;padding:2rem;background:linear-gradient(135deg,#0a0a14 0%,#16213e 50%,#0f3460 100%);position:relative;overflow:hidden;}" \
".hero::before{content:'';position:absolute;top:0;left:0;right:0;bottom:0;background:radial-gradient(ellipse at top,rgba(108,99,255,0.15) 0%,transparent 50%);}" \
".hero-content{position:relative;z-index:1;max-width:800px;}" \
".hero h1{font-size:3.5rem;font-weight:800;color:#fff;margin-bottom:1rem;letter-spacing:-1px;}" \
".hero h1 span{background:linear-gradient(135deg,#6c63ff,#43e97b);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;}" \
".hero .tagline{font-size:1.1rem;color:#8090a0;margin-bottom:0.5rem;letter-spacing:2px;text-transform:uppercase;}" \
".hero p{font-size:1.2rem;color:#b0b0c0;margin-bottom:2rem;}" \
".hero-buttons{display:flex;gap:1rem;justify-content:center;flex-wrap:wrap;}" \
".btn{display:inline-block;padding:14px 32px;border-radius:10px;font-weight:700;font-size:1rem;cursor:pointer;transition:all 0.2s;text-decoration:none;border:none;}" \
".btn-primary{background:linear-gradient(135deg,#6c63ff,#5a52e0);color:#fff;}" \
".btn-primary:hover{transform:translateY(-2px);box-shadow:0 10px 30px rgba(108,99,255,0.4);}" \
".btn-secondary{background:transparent;color:#fff;border:2px solid rgba(108,99,255,0.4);}" \
".btn-secondary:hover{border-color:#6c63ff;background:rgba(108,99,255,0.1);}" \
"/* Sections */" \
".section{padding:5rem 2rem;max-width:1200px;margin:0 auto;}" \
".section-title{text-align:center;font-size:2.2rem;font-weight:800;color:#fff;margin-bottom:0.5rem;}" \
".section-title span{color:#6c63ff;}" \
".section-sub{text-align:center;color:#808090;font-size:1rem;margin-bottom:3rem;}" \
"/* Cards Grid */" \
".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:1.5rem;}" \
".card{background:rgba(22,33,62,0.5);border:1px solid rgba(108,99,255,0.15);border-radius:16px;padding:2rem;transition:all 0.3s;}" \
".card:hover{transform:translateY(-4px);border-color:rgba(108,99,255,0.4);box-shadow:0 10px 40px rgba(0,0,0,0.3);}" \
".card-icon{font-size:2.5rem;margin-bottom:1rem;display:block;}" \
".card h3{color:#fff;font-size:1.2rem;margin-bottom:0.5rem;}" \
".card p{color:#9090a0;font-size:0.95rem;}" \
"/* Stats */" \
".stats{display:flex;justify-content:center;gap:3rem;flex-wrap:wrap;margin:3rem 0;}" \
".stat{text-align:center;}" \
".stat-num{font-size:2.5rem;font-weight:800;color:#6c63ff;}" \
".stat-label{color:#808090;font-size:0.9rem;text-transform:uppercase;letter-spacing:1px;}" \
"/* Table */" \
".table-wrap{overflow-x:auto;margin:2rem 0;}" \
"table{width:100%;border-collapse:collapse;}" \
"th,td{padding:12px 16px;text-align:left;border-bottom:1px solid rgba(108,99,255,0.1);}" \
"th{background:rgba(108,99,255,0.1);color:#6c63ff;font-weight:700;font-size:0.9rem;}" \
"td{color:#b0b0c0;font-size:0.9rem;}" \
"tr:hover{background:rgba(108,99,255,0.05);}" \
"/* Timeline */" \
".timeline{position:relative;padding-left:2rem;}" \
".timeline::before{content:'';position:absolute;left:8px;top:0;bottom:0;width:2px;background:linear-gradient(#6c63ff,#43e97b);}" \
".timeline-item{position:relative;padding-bottom:2rem;}" \
".timeline-item::before{content:'';position:absolute;left:-18px;top:6px;width:14px;height:14px;border-radius:50%;background:#6c63ff;border:2px solid #0a0a14;}" \
".timeline-item h4{color:#fff;font-size:1.1rem;margin-bottom:0.3rem;}" \
".timeline-item .year{color:#43e97b;font-weight:700;font-size:0.85rem;}" \
".timeline-item p{color:#9090a0;font-size:0.9rem;}" \
"/* Contact */" \
".contact-grid{display:grid;grid-template-columns:1fr 1fr;gap:3rem;}" \
".contact-info p{color:#b0b0c0;margin-bottom:1rem;}" \
".contact-info strong{color:#6c63ff;}" \
".contact-form input,.contact-form textarea{width:100%;padding:12px 16px;margin-bottom:1rem;background:rgba(22,33,62,0.5);border:1px solid rgba(108,99,255,0.2);border-radius:8px;color:#fff;font-size:0.95rem;}" \
".contact-form input:focus,.contact-form textarea:focus{outline:none;border-color:#6c63ff;}" \
".contact-form textarea{min-height:120px;resize:vertical;}" \
"/* Footer */" \
".footer{background:#06060d;border-top:1px solid rgba(108,99,255,0.1);padding:3rem 2rem;text-align:center;color:#606070;}" \
".footer a{color:#808090;}" \
".footer-links{display:flex;gap:1.5rem;justify-content:center;margin-bottom:1rem;}" \
".footer-bottom{font-size:0.85rem;}" \
"/* Responsive */" \
"@media(max-width:768px){" \
".nav{display:none;}" \
".hero h1{font-size:2rem;}" \
".hero p{font-size:1rem;}" \
".section{padding:3rem 1rem;}" \
".contact-grid{grid-template-columns:1fr;}" \
".stats{gap:1.5rem;}" \
".section-title{font-size:1.6rem;}" \
"}"

/* ------------------------------------------------------------------
 * Page generators — each returns a complete HTML page as a string
 * ------------------------------------------------------------------ */

char* page_home() {
    StrBuf sb; sb_init(&sb);

    sb_append(&sb, "<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    sb_append(&sb, "<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    sb_append(&sb, "<title>Afrika Integrated Holdings (AFRIH) - Pan-African Economic Group</title>");
    sb_append(&sb, "<meta name='description' content='AFRIH is a diversified Pan-African economic group mobilizing capital and developing enterprises across Africa.'>");
    sb_append(&sb, "<style>" CSS_STYLES "</style>");
    sb_append(&sb, "</head><body>");

    /* Header */
    sb_append(&sb, "<div class='header'><div class='header-inner'>");
    sb_append(&sb, "<div class='logo'>Afrika<span>Integrated</span> Holdings <span class='acronym'>AFRIH</span></div>");
    sb_append(&sb, "<div class='nav'>");
    sb_append(&sb, "<a href='/' class='active'>Home</a>");
    sb_append(&sb, "<a href='/about'>About</a>");
    sb_append(&sb, "<a href='/sectors'>Sectors</a>");
    sb_append(&sb, "<a href='/roadmap'>Roadmap</a>");
    sb_append(&sb, "<a href='/contact' class='nav-cta'>Contact</a>");
    sb_append(&sb, "</div></div></div>");

    /* Hero */
    sb_append(&sb, "<div class='hero'><div class='hero-content'>");
    sb_append(&sb, "<div class='tagline'>Pan-African Economic Group</div>");
    sb_append(&sb, "<h1>Afrika <span>Integrated</span> Holdings</h1>");
    sb_append(&sb, "<p>An integrated platform that mobilizes capital, develops businesses, and connects African value chains across countries and sectors.</p>");
    sb_append(&sb, "<div class='hero-buttons'>");
    sb_append(&sb, "<a href='/about' class='btn btn-primary'>Learn More</a>");
    sb_append(&sb, "<a href='/sectors' class='btn btn-secondary'>Our Sectors</a>");
    sb_append(&sb, "</div></div></div>");

    /* Stats */
    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<div class='stats'>");
    sb_append(&sb, "<div class='stat'><div class='stat-num'>12</div><div class='stat-label'>Economic Sectors</div></div>");
    sb_append(&sb, "<div class='stat'><div class='stat-num'>10</div><div class='stat-label'>Year Roadmap</div></div>");
    sb_append(&sb, "<div class='stat'><div class='stat-num'>$1B+</div><div class='stat-label'>Capital Vision</div></div>");
    sb_append(&sb, "<div class='stat'><div class='stat-num'>54</div><div class='stat-label'>African Nations</div></div>");
    sb_append(&sb, "</div></div>");

    /* What We Do */
    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<h2 class='section-title'>What <span>We Do</span></h2>");
    sb_append(&sb, "<p class='section-sub'>AFRIH operates as an ecosystem orchestrator across the African continent</p>");
    sb_append(&sb, "<div class='grid'>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>💰</span><h3>Capital Mobilization</h3><p>Source and deploy equity, debt, development finance, and project capital into bankable African opportunities.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🏢</span><h3>Enterprise Development</h3><p>Build and operate productive businesses across agriculture, energy, industry, infrastructure, and technology.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🔗</span><h3>Value-Chain Integration</h3><p>Connect African markets by linking production, processing, logistics, distribution, and finance into integrated corridors.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🤝</span><h3>Strategic Partnerships</h3><p>Form joint ventures with governments, local companies, multinationals, DFIs, and financial institutions.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>📊</span><h3>Governance & Risk</h3><p>Institutional-grade governance, risk management, and transparent reporting designed to earn investor trust.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🌱</span><h3>Impact & ESG</h3><p>Measurable impact on jobs, local value creation, energy access, and inclusion across all portfolio companies.</p></div>");
    sb_append(&sb, "</div></div>");

    /* Architecture snippet */
    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<h2 class='section-title'>Group <span>Architecture</span></h2>");
    sb_append(&sb, "<p class='section-sub'>A holding-company structure with specialized entities beneath</p>");
    sb_append(&sb, "<div class='table-wrap'><table>");
    sb_append(&sb, "<tr><th>Layer</th><th>Role</th></tr>");
    sb_append(&sb, "<tr><td>Parent / Holding Company</td><td>Strategy, portfolio allocation, governance, treasury, investor relations</td></tr>");
    sb_append(&sb, "<tr><td>Sector Platforms</td><td>Operating businesses and sector-specific subsidiaries</td></tr>");
    sb_append(&sb, "<tr><td>Capital &amp; Funds</td><td>Investment vehicles that raise and deploy capital into defined strategies</td></tr>");
    sb_append(&sb, "<tr><td>Country Platforms</td><td>Local operating and partnership structures</td></tr>");
    sb_append(&sb, "<tr><td>Project SPVs</td><td>Ring-fenced vehicles for major infrastructure and industrial projects</td></tr>");
    sb_append(&sb, "</table></div></div>");

    /* Footer */
    sb_append(&sb, "<div class='footer'>");
    sb_append(&sb, "<div class='footer-links'>");
    sb_append(&sb, "<a href='/'>Home</a><a href='/about'>About</a><a href='/sectors'>Sectors</a><a href='/roadmap'>Roadmap</a><a href='/contact'>Contact</a>");
    sb_append(&sb, "</div>");
    sb_append(&sb, "<div class='footer-bottom'>Afrika Integrated Holdings (AFRIH) - Concept Master Plan - Built in C</div>");
    sb_append(&sb, "</div>");

    sb_append(&sb, "</body></html>");
    return sb.data;
}

char* page_about() {
    StrBuf sb; sb_init(&sb);
    sb_append(&sb, "<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    sb_append(&sb, "<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    sb_append(&sb, "<title>About - Afrika Integrated Holdings (AFRIH)</title>");
    sb_append(&sb, "<style>" CSS_STYLES "</style>");
    sb_append(&sb, "</head><body>");

    sb_append(&sb, "<div class='header'><div class='header-inner'>");
    sb_append(&sb, "<div class='logo'>Afrika<span>Integrated</span> Holdings <span class='acronym'>AFRIH</span></div>");
    sb_append(&sb, "<div class='nav'>");
    sb_append(&sb, "<a href='/'>Home</a>");
    sb_append(&sb, "<a href='/about' class='active'>About</a>");
    sb_append(&sb, "<a href='/sectors'>Sectors</a>");
    sb_append(&sb, "<a href='/roadmap'>Roadmap</a>");
    sb_append(&sb, "<a href='/contact' class='nav-cta'>Contact</a>");
    sb_append(&sb, "</div></div></div>");

    sb_append(&sb, "<div class='hero' style='min-height:50vh;'>");
    sb_append(&sb, "<div class='hero-content'>");
    sb_append(&sb, "<h1>About <span>AFRIH</span></h1>");
    sb_append(&sb, "<p>Turning a Pan-African vision into a repeatable capital-and-enterprise machine.</p>");
    sb_append(&sb, "</div></div>");

    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<h2 class='section-title'>Our <span>Vision</span></h2>");
    sb_append(&sb, "<p class='section-sub'>Build a connected African economic platform that mobilizes capital and develops productive enterprises across the continent.</p>");

    sb_append(&sb, "<h2 class='section-title' style='margin-top:3rem;'>Our <span>Mission</span></h2>");
    sb_append(&sb, "<p class='section-sub'>Create investable businesses and infrastructure, connect African markets, strengthen local value chains, and recycle economic value into further African growth.</p>");

    sb_append(&sb, "<h2 class='section-title' style='margin-top:3rem;'>Strategic <span>Thesis</span></h2>");
    sb_append(&sb, "<p class='section-sub'>Africa's economic opportunities are constrained not by the absence of resources or entrepreneurs, but by fragmented capital, infrastructure, markets, and institutional execution. A group that connects these layers can create stronger economics than isolated projects.</p>");
    sb_append(&sb, "</div>");

    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<h2 class='section-title'>Core <span>Principles</span></h2>");
    sb_append(&sb, "<div class='grid'>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🎯</span><h3>Execution Before Scale</h3><p>Prove a small number of businesses before expanding the portfolio.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>⚖️</span><h3>Capital Matching</h3><p>Use equity, debt, grants, PPPs and project finance according to the cash-flow profile.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🤝</span><h3>Local Partnership</h3><p>Enter countries through credible local operators, institutions and community relationships.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🔗</span><h3>Value-Chain Integration</h3><p>Prefer businesses that strengthen other businesses in the group.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🛡️</span><h3>Governance First</h3><p>Investor trust is an asset; controls must grow ahead of complexity.</p></div>");
    sb_append(&sb, "<div class='card'><span class='card-icon'>🔄</span><h3>Reinvestment</h3><p>Use a disciplined share of profits to fund the next generation of growth.</p></div>");
    sb_append(&sb, "</div></div>");

    sb_append(&sb, "<div class='footer'><div class='footer-links'>");
    sb_append(&sb, "<a href='/'>Home</a><a href='/about'>About</a><a href='/sectors'>Sectors</a><a href='/roadmap'>Roadmap</a><a href='/contact'>Contact</a>");
    sb_append(&sb, "</div><div class='footer-bottom'>Afrika Integrated Holdings (AFRIH)</div></div>");

    sb_append(&sb, "</body></html>");
    return sb.data;
}

char* page_sectors() {
    StrBuf sb; sb_init(&sb);
    sb_append(&sb, "<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    sb_append(&sb, "<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    sb_append(&sb, "<title>Sectors - Afrika Integrated Holdings (AFRIH)</title>");
    sb_append(&sb, "<style>" CSS_STYLES "</style>");
    sb_append(&sb, "</head><body>");

    sb_append(&sb, "<div class='header'><div class='header-inner'>");
    sb_append(&sb, "<div class='logo'>Afrika<span>Integrated</span> Holdings <span class='acronym'>AFRIH</span></div>");
    sb_append(&sb, "<div class='nav'>");
    sb_append(&sb, "<a href='/'>Home</a><a href='/about'>About</a>");
    sb_append(&sb, "<a href='/sectors' class='active'>Sectors</a>");
    sb_append(&sb, "<a href='/roadmap'>Roadmap</a>");
    sb_append(&sb, "<a href='/contact' class='nav-cta'>Contact</a>");
    sb_append(&sb, "</div></div></div>");

    sb_append(&sb, "<div class='hero' style='min-height:50vh;'>");
    sb_append(&sb, "<div class='hero-content'>");
    sb_append(&sb, "<h1>Economic <span>Sectors</span></h1>");
    sb_append(&sb, "<p>12 sector platforms building Africa's productive capacity</p>");
    sb_append(&sb, "</div></div>");

    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<div class='grid'>");

    const char *sectors[] = {
        "🌾", "Agriculture & Food", "Farming, livestock, fisheries, storage, processing, exports. Food security and value addition.",
        "⚡", "Energy", "Solar, hydro, transmission, distributed energy, energy services. Power is an input into nearly every sector.",
        "🏭", "Industry", "Manufacturing, materials, machinery, consumer goods. Captures more value locally.",
        "🚧", "Infrastructure", "Roads, rail, ports, airports, water, industrial zones. Enables trade and productivity.",
        "🚚", "Logistics & Trade", "Warehousing, trucking, shipping, distribution, e-commerce. Connects producers to markets.",
        "🏦", "Finance", "Investment, fintech, insurance, trade finance. Mobilizes and allocates capital.",
        "💻", "Technology", "Software, data, AI, telecom, digital platforms. Improves productivity and coordination.",
        "🏥", "Healthcare", "Hospitals, diagnostics, pharmaceuticals, equipment. Human-capital resilience.",
        "🎓", "Education", "Schools, vocational training, universities, research. Builds the talent pipeline.",
        "🏠", "Housing & Cities", "Residential, commercial, urban infrastructure. Supports urbanization.",
        "✈️", "Tourism & Services", "Hotels, travel, entertainment, cultural economy. Foreign exchange and jobs.",
        "📋", "Cross-Sector", "Value-chain integration linking production, processing, logistics, finance, and technology.",
    };

    for (int i = 0; i < 12; i++) {
        sb_append(&sb, "<div class='card'><span class='card-icon'>");
        sb_append(&sb, sectors[i*3]);
        sb_append(&sb, "</span><h3>");
        sb_append(&sb, sectors[i*3+1]);
        sb_append(&sb, "</h3><p>");
        sb_append(&sb, sectors[i*3+2]);
        sb_append(&sb, "</p></div>");
    }

    sb_append(&sb, "</div></div>");

    /* Capital Architecture */
    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<h2 class='section-title'>Capital <span>Architecture</span></h2>");
    sb_append(&sb, "<p class='section-sub'>A layered capital model matching maturity of capital to maturity of asset</p>");
    sb_append(&sb, "<div class='table-wrap'><table>");
    sb_append(&sb, "<tr><th>Capital Type</th><th>Typical Use</th><th>Risk</th></tr>");
    sb_append(&sb, "<tr><td>Founder/Strategic Equity</td><td>Formation, early pilots, acquisitions</td><td>High</td></tr>");
    sb_append(&sb, "<tr><td>Institutional Equity</td><td>Scale and growth</td><td>Medium-High</td></tr>");
    sb_append(&sb, "<tr><td>DFI Capital</td><td>Development and infrastructure</td><td>Medium</td></tr>");
    sb_append(&sb, "<tr><td>Commercial Debt</td><td>Working capital, equipment, expansion</td><td>Medium</td></tr>");
    sb_append(&sb, "<tr><td>Project Finance</td><td>Large ring-fenced assets</td><td>Project-specific</td></tr>");
    sb_append(&sb, "<tr><td>Bonds/Capital Markets</td><td>Mature expansion</td><td>Market-dependent</td></tr>");
    sb_append(&sb, "<tr><td>Retained Earnings</td><td>Reinvestment</td><td>Lower</td></tr>");
    sb_append(&sb, "</table></div></div>");

    sb_append(&sb, "<div class='footer'><div class='footer-links'>");
    sb_append(&sb, "<a href='/'>Home</a><a href='/about'>About</a><a href='/sectors'>Sectors</a><a href='/roadmap'>Roadmap</a><a href='/contact'>Contact</a>");
    sb_append(&sb, "</div><div class='footer-bottom'>Afrika Integrated Holdings (AFRIH)</div></div>");

    sb_append(&sb, "</body></html>");
    return sb.data;
}

char* page_roadmap() {
    StrBuf sb; sb_init(&sb);
    sb_append(&sb, "<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    sb_append(&sb, "<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    sb_append(&sb, "<title>Roadmap - Afrika Integrated Holdings (AFRIH)</title>");
    sb_append(&sb, "<style>" CSS_STYLES "</style>");
    sb_append(&sb, "</head><body>");

    sb_append(&sb, "<div class='header'><div class='header-inner'>");
    sb_append(&sb, "<div class='logo'>Afrika<span>Integrated</span> Holdings <span class='acronym'>AFRIH</span></div>");
    sb_append(&sb, "<div class='nav'>");
    sb_append(&sb, "<a href='/'>Home</a><a href='/about'>About</a><a href='/sectors'>Sectors</a>");
    sb_append(&sb, "<a href='/roadmap' class='active'>Roadmap</a>");
    sb_append(&sb, "<a href='/contact' class='nav-cta'>Contact</a>");
    sb_append(&sb, "</div></div></div>");

    sb_append(&sb, "<div class='hero' style='min-height:50vh;'>");
    sb_append(&sb, "<div class='hero-content'>");
    sb_append(&sb, "<h1>10-Year <span>Roadmap</span></h1>");
    sb_append(&sb, "<p>Milestone-driven growth: Prove, Institutionalize, Scale, Recycle Capital</p>");
    sb_append(&sb, "</div></div>");

    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<h2 class='section-title'>Strategic <span>Timeline</span></h2>");
    sb_append(&sb, "<div class='timeline'>");

    const char *phases[] = {
        "Year 1", "Formation + Pilots", "Parent structure, governance, first capital, first 3-5 pilot projects.",
        "Years 2-3", "Proof + Fundraising Engine", "Operating revenue, investor pipeline, repeatable project model, dedicated sector subsidiaries.",
        "Years 3-5", "Sector Expansion", "Multiple subsidiaries, JV pipeline, DFI relationships, project-finance templates.",
        "Years 5-7", "Regional Scale", "Country platforms, larger funds, project-finance capability, cross-border trade.",
        "Years 7-10", "Institutional Scale", "Capital markets access, major infrastructure portfolio, mature governance, capital recycling.",
    };

    for (int i = 0; i < 5; i++) {
        sb_append(&sb, "<div class='timeline-item'>");
        sb_append(&sb, "<div class='year'>");
        sb_append(&sb, phases[i*3]);
        sb_append(&sb, "</div><h4>");
        sb_append(&sb, phases[i*3+1]);
        sb_append(&sb, "</h4><p>");
        sb_append(&sb, phases[i*3+2]);
        sb_append(&sb, "</p></div>");
    }

    sb_append(&sb, "</div></div>");

    /* Fundraising targets */
    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<h2 class='section-title'>Fundraising <span>Targets</span></h2>");
    sb_append(&sb, "<p class='section-sub'>Illustrative capital objectives by stage</p>");
    sb_append(&sb, "<div class='table-wrap'><table>");
    sb_append(&sb, "<tr><th>Stage</th><th>Capital Objective</th><th>Primary Sources</th></tr>");
    sb_append(&sb, "<tr><td>Formation</td><td>$1M - $10M</td><td>Founders, strategic investors</td></tr>");
    sb_append(&sb, "<tr><td>Early Scale</td><td>$10M - $50M</td><td>Family offices, diaspora, strategic capital</td></tr>");
    sb_append(&sb, "<tr><td>Institutional Scale</td><td>$50M - $250M</td><td>Institutions, DFIs, PE</td></tr>");
    sb_append(&sb, "<tr><td>Large Projects</td><td>$100M - $1B+ per project</td><td>Project finance, JV, DFI, banks</td></tr>");
    sb_append(&sb, "<tr><td>Mature Markets</td><td>Case-specific</td><td>Bonds, funds, public markets</td></tr>");
    sb_append(&sb, "</table></div></div>");

    /* Governance */
    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<h2 class='section-title'>Governance <span>Model</span></h2>");
    sb_append(&sb, "<p class='section-sub'>Trust is a financing asset. Governance is designed before the organization becomes large.</p>");
    sb_append(&sb, "<div class='table-wrap'><table>");
    sb_append(&sb, "<tr><th>Body</th><th>Responsibility</th></tr>");
    sb_append(&sb, "<tr><td>Board</td><td>Strategy, oversight, CEO appointment, major transactions</td></tr>");
    sb_append(&sb, "<tr><td>Audit &amp; Risk Committee</td><td>Financial reporting, controls, risk, audit</td></tr>");
    sb_append(&sb, "<tr><td>Investment Committee</td><td>Capital allocation and investment approvals</td></tr>");
    sb_append(&sb, "<tr><td>Credit/Finance Committee</td><td>Debt, guarantees, liquidity and treasury</td></tr>");
    sb_append(&sb, "<tr><td>ESG / Impact Committee</td><td>Environmental, social and impact standards</td></tr>");
    sb_append(&sb, "<tr><td>Internal Audit</td><td>Independent control testing</td></tr>");
    sb_append(&sb, "</table></div></div>");

    sb_append(&sb, "<div class='footer'><div class='footer-links'>");
    sb_append(&sb, "<a href='/'>Home</a><a href='/about'>About</a><a href='/sectors'>Sectors</a><a href='/roadmap'>Roadmap</a><a href='/contact'>Contact</a>");
    sb_append(&sb, "</div><div class='footer-bottom'>Afrika Integrated Holdings (AFRIH)</div></div>");

    sb_append(&sb, "</body></html>");
    return sb.data;
}

char* page_contact() {
    StrBuf sb; sb_init(&sb);
    sb_append(&sb, "<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    sb_append(&sb, "<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    sb_append(&sb, "<title>Contact - Afrika Integrated Holdings (AFRIH)</title>");
    sb_append(&sb, "<style>" CSS_STYLES "</style>");
    sb_append(&sb, "</head><body>");

    sb_append(&sb, "<div class='header'><div class='header-inner'>");
    sb_append(&sb, "<div class='logo'>Afrika<span>Integrated</span> Holdings <span class='acronym'>AFRIH</span></div>");
    sb_append(&sb, "<div class='nav'>");
    sb_append(&sb, "<a href='/'>Home</a><a href='/about'>About</a><a href='/sectors'>Sectors</a><a href='/roadmap'>Roadmap</a>");
    sb_append(&sb, "<a href='/contact' class='active nav-cta'>Contact</a>");
    sb_append(&sb, "</div></div></div>");

    sb_append(&sb, "<div class='hero' style='min-height:50vh;'>");
    sb_append(&sb, "<div class='hero-content'>");
    sb_append(&sb, "<h1>Get <span>In Touch</span></h1>");
    sb_append(&sb, "<p>Partner with AFRIH to build Africa's economic future.</p>");
    sb_append(&sb, "</div></div>");

    sb_append(&sb, "<div class='section'>");
    sb_append(&sb, "<div class='contact-grid'>");
    sb_append(&sb, "<div class='contact-info'>");
    sb_append(&sb, "<h2 class='section-title' style='text-align:left;'>Contact <span>Details</span></h2>");
    sb_append(&sb, "<p><strong>Organization:</strong> Afrika Integrated Holdings (AFRIH)</p>");
    sb_append(&sb, "<p><strong>Status:</strong> Concept Master Plan stage</p>");
    sb_append(&sb, "<p><strong>Sectors:</strong> Agriculture, Energy, Industry, Infrastructure, Finance, Technology, Logistics, Healthcare, Education, Housing, Tourism, Trade</p>");
    sb_append(&sb, "<p><strong>Geography:</strong> Pan-African, multi-country platform</p>");
    sb_append(&sb, "<p><strong>Capital Vision:</strong> $1B+ across funds, projects, and operating companies over 10 years</p>");
    sb_append(&sb, "<p><strong>Partnership Types:</strong> Strategic investors, DFIs, banks, diaspora capital, government/PPP, corporate partners</p>");
    sb_append(&sb, "</div>");
    sb_append(&sb, "<div class='contact-form'>");
    sb_append(&sb, "<h2 class='section-title' style='text-align:left;'>Send a <span>Message</span></h2>");
    sb_append(&sb, "<form action='/submit' method='POST'>");
    sb_append(&sb, "<input type='text' name='name' placeholder='Your Name' required>");
    sb_append(&sb, "<input type='email' name='email' placeholder='Your Email' required>");
    sb_append(&sb, "<input type='text' name='organization' placeholder='Organization'>");
    sb_append(&sb, "<input type='text' name='interest' placeholder='Partnership Interest (e.g. Investor, DFI, JV Partner)'>");
    sb_append(&sb, "<textarea name='message' placeholder='Your Message' required></textarea>");
    sb_append(&sb, "<button type='submit' class='btn btn-primary' style='width:100%;'>Submit</button>");
    sb_append(&sb, "</form></div>");
    sb_append(&sb, "</div></div>");

    sb_append(&sb, "<div class='footer'><div class='footer-links'>");
    sb_append(&sb, "<a href='/'>Home</a><a href='/about'>About</a><a href='/sectors'>Sectors</a><a href='/roadmap'>Roadmap</a><a href='/contact'>Contact</a>");
    sb_append(&sb, "</div><div class='footer-bottom'>Afrika Integrated Holdings (AFRIH) - Concept Master Plan - Not a legal or investment offering</div></div>");

    sb_append(&sb, "</body></html>");
    return sb.data;
}

char* page_404() {
    StrBuf sb; sb_init(&sb);
    sb_append(&sb, "<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    sb_append(&sb, "<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    sb_append(&sb, "<title>404 - AFRIH</title>");
    sb_append(&sb, "<style>" CSS_STYLES "</style>");
    sb_append(&sb, "</head><body>");
    sb_append(&sb, "<div class='hero'><div class='hero-content'>");
    sb_append(&sb, "<h1>404</h1>");
    sb_append(&sb, "<p>Page not found.</p>");
    sb_append(&sb, "<a href='/' class='btn btn-primary'>Back to Home</a>");
    sb_append(&sb, "</div></div>");
    sb_append(&sb, "</body></html>");
    return sb.data;
}

char* page_submit_success() {
    StrBuf sb; sb_init(&sb);
    sb_append(&sb, "<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    sb_append(&sb, "<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    sb_append(&sb, "<title>Message Received - AFRIH</title>");
    sb_append(&sb, "<style>" CSS_STYLES "</style>");
    sb_append(&sb, "</head><body>");
    sb_append(&sb, "<div class='hero'><div class='hero-content'>");
    sb_append(&sb, "<h1>Message <span>Received</span></h1>");
    sb_append(&sb, "<p>Thank you for reaching out. We will respond to your inquiry shortly.</p>");
    sb_append(&sb, "<a href='/' class='btn btn-primary'>Back to Home</a>");
    sb_append(&sb, "</div></div>");
    sb_append(&sb, "</body></html>");
    return sb.data;
}

/* ------------------------------------------------------------------
 * Route table
 * ------------------------------------------------------------------ */
typedef struct {
    const char *path;
    const char *method;
    char* (*handler)(void);
} Route;

Route routes[] = {
    {"/",            "GET",  page_home},
    {"/about",       "GET",  page_about},
    {"/sectors",     "GET",  page_sectors},
    {"/roadmap",     "GET",  page_roadmap},
    {"/contact",     "GET",  page_contact},
    {"/submit",      "POST", page_submit_success},
};

const int route_count = sizeof(routes) / sizeof(routes[0]);

/* ------------------------------------------------------------------
 * HTTP response sender
 * ------------------------------------------------------------------ */
void send_response(int client_fd, int status, const char *content_type, const char *body) {
    StrBuf resp; sb_init(&resp);

    const char *status_text = (status == 200) ? "200 OK" : "404 Not Found";

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s; charset=UTF-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Server: AFRIH-C-Server/1.0\r\n"
        "\r\n",
        status_text, content_type, strlen(body)
    );

    sb_append(&resp, header);
    sb_append(&resp, body);

    send(client_fd, resp.data, resp.len, 0);
    sb_free(&resp);
}

/* ------------------------------------------------------------------
 * Parse the HTTP request line to get method and path
 * ------------------------------------------------------------------ */
void parse_request(const char *raw, char *method, char *path) {
    const char *m_start = raw;
    const char *space1 = strchr(raw, ' ');
    if (!space1) { strcpy(method, "GET"); strcpy(path, "/"); return; }

    size_t mlen = space1 - m_start;
    if (mlen >= 15) mlen = 14;
    strncpy(method, m_start, mlen);
    method[mlen] = '\0';

    const char *p_start = space1 + 1;
    const char *space2 = strchr(p_start, ' ');
    if (!space2) { strcpy(path, "/"); return; }

    /* Strip query string */
    size_t plen = space2 - p_start;
    const char *qmark = memchr(p_start, '?', plen);
    if (qmark) plen = qmark - p_start;

    if (plen >= 255) plen = 254;
    strncpy(path, p_start, plen);
    path[plen] = '\0';
}

/* ------------------------------------------------------------------
 * Handle a single client connection
 * ------------------------------------------------------------------ */
void* handle_client(void *arg) {
    int client_fd = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        close(client_fd);
        return NULL;
    }

    char method[16], path[256];
    parse_request(buffer, method, path);

    printf("[AFRIH] %s %s\n", method, path);

    /* Match route */
    int found = 0;
    for (int i = 0; i < route_count; i++) {
        if (strcmp(path, routes[i].path) == 0 && strcmp(method, routes[i].method) == 0) {
            char *html = routes[i].handler();
            send_response(client_fd, 200, "text/html", html);
            free(html);
            found = 1;
            break;
        }
    }

    if (!found) {
        char *html = page_404();
        send_response(client_fd, 404, "text/html", html);
        free(html);
    }

    close(client_fd);
    return NULL;
}

/* ------------------------------------------------------------------
 * Main — start the server
 * ------------------------------------------------------------------ */
int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_fd, 50) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("\n");
    printf("============================================\n");
    printf("  AFRIKA INTEGRATED HOLDINGS (AFRIH)\n");
    printf("  Web Server - Built in C\n");
    printf("============================================\n");
    printf("  Listening on port %d\n", PORT);
    printf("  Open: http://localhost:%d\n", PORT);
    printf("  Routes:\n");
    printf("    GET  /          - Home page\n");
    printf("    GET  /about     - About page\n");
    printf("    GET  /sectors   - Economic sectors\n");
    printf("    GET  /roadmap   - 10-year roadmap\n");
    printf("    GET  /contact   - Contact form\n");
    printf("    POST /submit    - Form submission\n");
    printf("============================================\n");
    printf("  Press Ctrl+C to stop.\n\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));

        *client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (*client_fd < 0) {
            perror("Accept failed");
            free(client_fd);
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_fd);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}
