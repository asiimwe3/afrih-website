/*
 * ===================================================================
 * Afrika Integrated Holdings (AFRIH) — Full Web Server in C
 * All 11 pages. No HTML files. No JS frameworks. Pure C + sockets.
 * ===================================================================
 * Compile: gcc -O2 -o afrih_server afrih_server.c -lpthread
 * Run:     ./afrih_server
 * Open:    http://localhost:8080
 * ===================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>

#define PORT 8080
#define BUFSZ 65536
#define MAXREQ 8192
#define MAXPATH 2048

/* ── String builder ──────────────────────────────────────────── */
typedef struct { char *data; size_t len, cap; } StrBuf;

static void sb_init(StrBuf *sb) {
    sb->cap = 8192; sb->len = 0;
    sb->data = malloc(sb->cap);
    sb->data[0] = '\0';
}

static void sb_append(StrBuf *sb, const char *s) {
    size_t l = strlen(s);
    if (sb->len + l + 1 >= sb->cap) {
        while (sb->len + l + 1 >= sb->cap) sb->cap *= 2;
        sb->data = realloc(sb->data, sb->cap);
    }
    memcpy(sb->data + sb->len, s, l);
    sb->len += l;
    sb->data[sb->len] = '\0';
}

static void sb_appendf(StrBuf *sb, const char *fmt, ...) {
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) sb_append(sb, buf);
}

/* ── CSS (all styling, hand-ported from the design) ───────────── */
static const char *CSS =
    ":root{--navy:#0a0a14;--navy2:#0f1220;--card:#12152a;--purple:#6c63ff;--purple2:#5a52e0;--green:#43e97b;--text:#e8e8f0;--text2:#a0a0b8;--text3:#6a6a80;--border:rgba(108,99,255,0.15);--radius:14px}\n*,*::before,*::after{margin:0;padding:0;box-sizing:border-box}\nhtml{scroll-behavior:smooth}\nbody{font-family:'Inter',system-ui,sans-serif;background:var(--navy);color:var(--text);line-height:1.7;overflow-x:hidden}\na{color:var(--purple);text-decoration:none;cursor:pointer}\nimg{max-width:100%;display:block}\n\n/* HEADER */\n.header{position:fixed;top:0;left:0;right:0;z-index:100;background:rgba(10,10,20,0.85);backdrop-filter:blur(20px);border-bottom:1px solid var(--border);transition:padding 0.3s}\n.header.scrolled{padding:0 0}\n.header-inner{max-width:1280px;margin:0 auto;display:flex;align-items:center;justify-content:space-between;height:72px;padding:0 2rem}\n.logo{font-size:1.35rem;font-weight:800;color:#fff;letter-spacing:-0.5px;display:flex;align-items:center;gap:8px;cursor:pointer}\n.logo-mark{width:36px;height:36px;border-radius:10px;background:linear-gradient(135deg,var(--purple),var(--green));display:flex;align-items:center;justify-content:center;font-size:1rem;font-weight:900;color:#fff}\n.logo span{color:var(--purple)}\n.logo .acronym{font-size:0.7rem;color:var(--green);font-weight:700;letter-spacing:2px;margin-left:4px}\n.nav{display:flex;gap:0.3rem;align-items:center}\n.nav a{color:var(--text2);font-size:0.85rem;font-weight:500;padding:8px 14px;border-radius:8px;transition:all 0.2s}\n.nav a:hover{color:#fff;background:rgba(108,99,255,0.08)}\n.nav a.active{color:var(--purple);background:rgba(108,99,255,0.1)}\n.nav-cta{background:linear-gradient(135deg,var(--purple),var(--purple2));color:#fff!important;padding:10px 22px!important;border-radius:10px;font-weight:600}\n.nav-cta:hover{transform:translateY(-1px);box-shadow:0 6px 20px rgba(108,99,255,0.35);background:linear-gradient(135deg,var(--purple),var(--purple2))!important}\n.menu-btn{display:none;background:none;border:none;color:#fff;font-size:1.4rem;cursor:pointer}\n\n/* PAGES */\n.page{display:none;animation:fadeUp 0.5s ease}\n.page.active{display:block}\n@keyframes fadeUp{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}\n\n/* HERO */\n.hero{min-height:100vh;display:flex;align-items:center;justify-content:center;text-align:center;padding:6rem 2rem 3rem;position:relative;overflow:hidden}\n.hero-bg{position:absolute;inset:0;z-index:0}\n.hero-bg img{width:100%;height:100%;object-fit:cover;opacity:0.35}\n.hero-bg::after{content:'';position:absolute;inset:0;background:linear-gradient(180deg,rgba(10,10,20,0.6)0%,rgba(10,10,20,0.85)100%)}\n.hero-content{position:relative;z-index:1;max-width:900px}\n.hero-badge{display:inline-flex;align-items:center;gap:8px;background:rgba(108,99,255,0.12);border:1px solid rgba(108,99,255,0.25);padding:8px 18px;border-radius:100px;font-size:0.8rem;font-weight:600;color:var(--purple);margin-bottom:1.5rem;letter-spacing:0.5px}\n.hero-badge .dot{width:8px;height:8px;border-radius:50%;background:var(--green);animation:pulse 2s infinite}\n@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.4}}\n.hero h1{font-size:3.8rem;font-weight:800;color:#fff;margin-bottom:1.2rem;letter-spacing:-1.5px;line-height:1.1}\n.hero h1 span{background:linear-gradient(135deg,var(--purple),var(--green));-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}\n.hero .tagline{font-size:0.95rem;color:var(--green);margin-bottom:0.8rem;letter-spacing:3px;text-transform:uppercase;font-weight:600}\n.hero p{font-size:1.15rem;color:var(--text2);margin-bottom:2.5rem;max-width:700px;margin-left:auto;margin-right:auto}\n.hero-buttons{display:flex;gap:1rem;justify-content:center;flex-wrap:wrap}\n.btn{display:inline-flex;align-items:center;gap:8px;padding:14px 32px;border-radius:12px;font-weight:700;font-size:0.95rem;cursor:pointer;transition:all 0.25s;border:none;font-family:inherit}\n.btn-primary{background:linear-gradient(135deg,var(--purple),var(--purple2));color:#fff}\n.btn-primary:hover{transform:translateY(-2px);box-shadow:0 12px 36px rgba(108,99,255,0.4)}\n.btn-secondary{background:rgba(255,255,255,0.05);color:#fff;border:1px solid rgba(255,255,255,0.15)}\n.btn-secondary:hover{border-color:var(--purple);background:rgba(108,99,255,0.08)}\n.btn-green{background:linear-gradient(135deg,var(--green),#38d972);color:#0a0a14}\n.btn-green:hover{transform:translateY(-2px);box-shadow:0 12px 36px rgba(67,233,123,0.35)}\n\n/* STATS BAR */\n.stats-bar{background:linear-gradient(135deg,var(--navy2),var(--card));border-top:1px solid var(--border);border-bottom:1px solid var(--border);padding:2.5rem 2rem}\n.stats-bar-inner{max-width:1280px;margin:0 auto;display:flex;justify-content:space-around;flex-wrap:wrap;gap:2rem}\n.stat{text-align:center}\n.stat-num{font-size:2.5rem;font-weight:800;background:linear-gradient(135deg,var(--purple),var(--green));-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}\n.stat-label{color:var(--text3);font-size:0.8rem;text-transform:uppercase;letter-spacing:1.5px;margin-top:4px;font-weight:600}\n\n/* SECTIONS */\n.section{padding:5rem 2rem;max-width:1280px;margin:0 auto}\n.section-title{text-align:center;font-size:2.3rem;font-weight:800;color:#fff;margin-bottom:0.6rem;letter-spacing:-0.5px}\n.section-title span{color:var(--purple)}\n.section-sub{text-align:center;color:var(--text2);font-size:1.05rem;margin-bottom:3.5rem;max-width:650px;margin-left:auto;margin-right:auto}\n.section-tag{text-align:center;display:inline-block;background:rgba(108,99,255,0.1);color:var(--purple);padding:6px 16px;border-radius:100px;font-size:0.75rem;font-weight:600;letter-spacing:1px;text-transform:uppercase;margin:0 auto 1rem}\n.section-tag-wrap{text-align:center;margin-bottom:1rem}\n\n/* CARDS */\n.grid{display:grid;gap:1.5rem}\n.grid-2{grid-template-columns:repeat(auto-fit,minmax(320px,1fr))}\n.grid-3{grid-template-columns:repeat(auto-fit,minmax(280px,1fr))}\n.grid-4{grid-template-columns:repeat(auto-fit,minmax(240px,1fr))}\n.card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:2rem;transition:all 0.3s}\n.card:hover{transform:translateY(-3px);border-color:rgba(108,99,255,0.35);box-shadow:0 12px 40px rgba(0,0,0,0.3)}\n.card-icon{width:52px;height:52px;border-radius:12px;background:rgba(108,99,255,0.1);display:flex;align-items:center;justify-content:center;font-size:1.5rem;margin-bottom:1.2rem}\n.card h3{color:#fff;font-size:1.15rem;margin-bottom:0.5rem;font-weight:700}\n.card p{color:var(--text2);font-size:0.9rem}\n.card-link{color:var(--purple);font-size:0.85rem;font-weight:600;margin-top:1rem;display:inline-block}\n.card-link:hover{color:var(--green)}\n\n/* FEATURE CARD (larger) */\n.feature-card{background:var(--card);border:1px solid var(--border);border-radius:18px;padding:2.5rem;transition:all 0.3s;position:relative;overflow:hidden}\n.feature-card::before{content:'';position:absolute;top:0;left:0;right:0;height:3px;background:linear-gradient(90deg,var(--purple),var(--green));opacity:0;transition:opacity 0.3s}\n.feature-card:hover::before{opacity:1}\n.feature-card:hover{transform:translateY(-4px);box-shadow:0 20px 50px rgba(0,0,0,0.4)}\n.feature-card .num{font-size:3rem;font-weight:800;color:rgba(108,99,255,0.15);position:absolute;top:1rem;right:1.5rem;line-height:1}\n.feature-card h3{color:#fff;font-size:1.25rem;margin-bottom:0.6rem;font-weight:700;position:relative}\n.feature-card p{color:var(--text2);font-size:0.92rem;position:relative}\n.feature-card ul{margin-top:1rem;padding-left:1.2rem;position:relative}\n.feature-card ul li{color:var(--text2);font-size:0.88rem;margin-bottom:0.4rem}\n.feature-card ul li::marker{color:var(--green)}\n\n/* TABLE */\n.table-wrap{overflow-x:auto;margin:2rem 0;border-radius:var(--radius);border:1px solid var(--border)}\ntable{width:100%;border-collapse:collapse}\nth{background:rgba(108,99,255,0.08);color:var(--purple);font-weight:700;font-size:0.82rem;padding:14px 18px;text-align:left;text-transform:uppercase;letter-spacing:0.5px}\ntd{padding:14px 18px;color:var(--text2);font-size:0.9rem;border-top:1px solid rgba(108,99,255,0.06)}\ntr:hover td{background:rgba(108,99,255,0.03)}\n\n/* TIMELINE */\n.timeline{position:relative;padding-left:2.5rem;max-width:800px;margin:0 auto}\n.timeline::before{content:'';position:absolute;left:10px;top:0;bottom:0;width:2px;background:linear-gradient(180deg,var(--purple),var(--green))}\n.timeline-item{position:relative;padding-bottom:2.5rem}\n.timeline-item::before{content:'';position:absolute;left:-22px;top:6px;width:16px;height:16px;border-radius:50%;background:var(--purple);border:3px solid var(--navy);box-shadow:0 0 12px rgba(108,99,255,0.5)}\n.timeline-item .year{color:var(--green);font-weight:700;font-size:0.8rem;letter-spacing:1px;text-transform:uppercase;margin-bottom:0.3rem}\n.timeline-item h4{color:#fff;font-size:1.15rem;margin-bottom:0.4rem;font-weight:700}\n.timeline-item p{color:var(--text2);font-size:0.9rem}\n.timeline-item .metrics{margin-top:0.8rem;display:flex;gap:1rem;flex-wrap:wrap}\n.timeline-item .metric{background:rgba(108,99,255,0.08);padding:4px 12px;border-radius:6px;font-size:0.78rem;color:var(--purple);font-weight:600}\n\n/* SPLIT SECTION */\n.split{display:grid;grid-template-columns:1fr 1fr;gap:3rem;align-items:center}\n.split-img{border-radius:18px;overflow:hidden;border:1px solid var(--border)}\n.split-img img{width:100%;height:400px;object-fit:cover}\n.split-content h2{font-size:1.8rem;font-weight:800;color:#fff;margin-bottom:1rem}\n.split-content p{color:var(--text2);margin-bottom:1rem}\n\n/* FORMS */\n.form-group{margin-bottom:1rem}\n.form-group label{display:block;color:var(--text2);font-size:0.82rem;margin-bottom:0.4rem;font-weight:600;letter-spacing:0.3px}\n.form-group input,.form-group select,.form-group textarea{width:100%;padding:13px 16px;background:var(--card);border:1px solid var(--border);border-radius:10px;color:#fff;font-size:0.92rem;font-family:inherit;transition:border 0.2s}\n.form-group input:focus,.form-group select:focus,.form-group textarea:focus{outline:none;border-color:var(--purple)}\n.form-group textarea{min-height:130px;resize:vertical}\n.form-group select option{background:var(--card)}\n.form-row{display:grid;grid-template-columns:1fr 1fr;gap:1rem}\n.alert{padding:14px 18px;border-radius:10px;margin-bottom:1.2rem;font-size:0.88rem;display:none;font-weight:500}\n.alert-success{background:rgba(67,233,123,0.12);border:1px solid rgba(67,233,123,0.3);color:var(--green)}\n.alert-error{background:rgba(255,80,80,0.12);border:1px solid rgba(255,80,80,0.3);color:#ff7070}\n.alert.show{display:block;animation:fadeUp 0.3s ease}\n.loading{display:inline-block;width:18px;height:18px;border:2.5px solid rgba(255,255,255,0.2);border-top-color:#fff;border-radius:50%;animation:spin 0.7s linear infinite;margin-right:8px;vertical-align:middle}\n@keyframes spin{to{transform:rotate(360deg)}}\n\n/* CONTACT */\n.contact-grid{display:grid;grid-template-columns:1fr 1.2fr;gap:3rem;max-width:1100px;margin:0 auto}\n.contact-info-card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:2.5rem}\n.contact-info-card h3{color:#fff;font-size:1.2rem;margin-bottom:1.5rem}\n.contact-info-card .info-row{display:flex;align-items:flex-start;gap:12px;margin-bottom:1.2rem}\n.contact-info-card .info-icon{width:40px;height:40px;border-radius:10px;background:rgba(108,99,255,0.1);display:flex;align-items:center;justify-content:center;font-size:1.1rem;flex-shrink:0}\n.contact-info-card .info-label{color:var(--text3);font-size:0.75rem;text-transform:uppercase;letter-spacing:1px;font-weight:600}\n.contact-info-card .info-value{color:var(--text);font-size:0.92rem;font-weight:500}\n\n/* NEWSLETTER */\n.newsletter-box{background:linear-gradient(135deg,rgba(108,99,255,0.08),rgba(67,233,123,0.05));border:1px solid var(--border);border-radius:18px;padding:3.5rem;text-align:center;margin:4rem auto;max-width:700px}\n.newsletter-box h3{color:#fff;font-size:1.5rem;margin-bottom:0.5rem;font-weight:700}\n.newsletter-box p{color:var(--text2);margin-bottom:1.5rem}\n.newsletter-form{display:flex;gap:0.6rem;justify-content:center;flex-wrap:wrap}\n.newsletter-form input{flex:1;min-width:220px;padding:13px 18px;background:var(--card);border:1px solid var(--border);border-radius:10px;color:#fff;font-size:0.92rem;font-family:inherit}\n.newsletter-form input:focus{outline:none;border-color:var(--purple)}\n\n/* FAQ */\n.faq-item{background:var(--card);border:1px solid var(--border);border-radius:12px;margin-bottom:0.8rem;overflow:hidden;transition:all 0.2s}\n.faq-item:hover{border-color:rgba(108,99,255,0.3)}\n.faq-q{padding:1.2rem 1.5rem;cursor:pointer;display:flex;justify-content:space-between;align-items:center;font-weight:600;color:#fff;font-size:0.95rem;user-select:none}\n.faq-q .arrow{color:var(--purple);transition:transform 0.3s;font-size:1.2rem}\n.faq-item.open .arrow{transform:rotate(45deg)}\n.faq-a{max-height:0;overflow:hidden;transition:max-height 0.3s ease;padding:0 1.5rem}\n.faq-item.open .faq-a{max-height:400px;padding:0 1.5rem 1.2rem}\n.faq-a p{color:var(--text2);font-size:0.9rem}\n\n/* CTA BANNER */\n.cta-banner{background:linear-gradient(135deg,var(--card),var(--navy2));border:1px solid var(--border);border-radius:20px;padding:4rem 3rem;text-align:center;margin:3rem auto;max-width:900px;position:relative;overflow:hidden}\n.cta-banner::before{content:'';position:absolute;top:-50%;right:-20%;width:400px;height:400px;border-radius:50%;background:radial-gradient(circle,rgba(108,99,255,0.08),transparent);filter:blur(40px)}\n.cta-banner h2{color:#fff;font-size:2rem;font-weight:800;margin-bottom:0.8rem;position:relative}\n.cta-banner p{color:var(--text2);margin-bottom:2rem;position:relative}\n.cta-banner .btn{position:relative}\n\n/* FOOTER */\n.footer{background:#06060d;border-top:1px solid var(--border);padding:4rem 2rem 2rem}\n.footer-inner{max-width:1280px;margin:0 auto}\n.footer-grid{display:grid;grid-template-columns:2fr 1fr 1fr 1fr;gap:3rem;margin-bottom:3rem}\n.footer-brand .logo{margin-bottom:1rem}\n.footer-brand p{color:var(--text3);font-size:0.88rem;max-width:320px}\n.footer-col h4{color:#fff;font-size:0.85rem;font-weight:700;margin-bottom:1rem;text-transform:uppercase;letter-spacing:1px}\n.footer-col a{display:block;color:var(--text2);font-size:0.88rem;margin-bottom:0.6rem;cursor:pointer;transition:color 0.2s}\n.footer-col a:hover{color:var(--purple)}\n.footer-bottom{border-top:1px solid var(--border);padding-top:2rem;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:1rem}\n.footer-bottom p{color:var(--text3);font-size:0.8rem}\n.footer-disclaimer{background:rgba(255,255,255,0.02);border:1px solid var(--border);border-radius:10px;padding:1.5rem;margin:2rem 0;border-left:3px solid var(--purple)}\n.footer-disclaimer p{color:var(--text3);font-size:0.78rem;line-height:1.5}\n\n/* BADGES */\n.badge{display:inline-block;padding:4px 12px;border-radius:100px;font-size:0.72rem;font-weight:600;letter-spacing:0.5px}\n.badge-purple{background:rgba(108,99,255,0.15);color:var(--purple)}\n.badge-green{background:rgba(67,233,123,0.15);color:var(--green)}\n.badge-blue{background:rgba(100,149,237,0.15);color:#6495ed}\n\n/* DIVIDER */\n.divider{height:1px;background:var(--border);margin:4rem 0}\n\n/* RESPONSIVE */\n@media(max-width:1024px){.footer-grid{grid-template-columns:1fr 1fr}.split{grid-template-columns:1fr}.contact-grid{grid-template-columns:1fr}}\n@media(max-width:768px){\n.nav{display:none;position:fixed;top:72px;left:0;right:0;background:rgba(10,10,20,0.98);backdrop-filter:blur(20px);flex-direction:column;padding:1.5rem 2rem;gap:0.5rem;border-bottom:1px solid var(--border)}\n.nav.open{display:flex}\n.nav a{width:100%;padding:12px 14px}\n.menu-btn{display:block}\n.hero h1{font-size:2.2rem}\n.hero p{font-size:1rem}\n.section{padding:3rem 1.2rem}\n.section-title{font-size:1.7rem}\n.stats-bar-inner{gap:1.5rem}\n.stat-num{font-size:1.8rem}\n.form-row{grid-template-columns:1fr}\n.footer-grid{grid-template-columns:1fr}\n.cta-banner{padding:2.5rem 1.5rem}\n.cta-banner h2{font-size:1.5rem}\n.split{grid-template-columns:1fr}\n.split-img img{height:250px}\n.feature-card{padding:1.8rem}\n}";

/* ── Shared HTML components ──────────────────────────────────── */

static const char *NAV =
    "<nav class=\"header\" id=\"header\">"
    "<div class=\"header-inner\">"
    "<a href=\"/\" class=\"logo\"><div class=\"logo-mark\">A</div>"
    "Afrika<span>Integrated</span> <span class=\"acronym\">AFRIH</span></a>"
    "<div class=\"nav\" id=\"nav\">"
    "<a href=\"/\" class=\"active\">Home</a>"
    "<a href=\"/thesis\">Investment Thesis</a>"
    "<a href=\"/model\">Business Model</a>"
    "<a href=\"/sectors\">Sectors</a>"
    "<a href=\"/governance\">Governance</a>"
    "<a href=\"/impact\">Impact</a>"
    "<a href=\"/roadmap\">Roadmap</a>"
    "<a href=\"/faq\">FAQ</a>"
    "<a href=\"/invest\" class=\"nav-cta\">Invest</a>"
    "</div>"
    "<button class=\"menu-btn\" onclick=\"document.getElementById('nav').classList.toggle('open')\">&#9776;</button>"
    "</div></nav>";

static const char *FOOTER =
    "<footer class=\"footer\"><div class=\"footer-inner\">"
    "<div class=\"footer-grid\">"
    "<div class=\"footer-brand\">"
    "<a href=\"/\" class=\"logo\"><div class=\"logo-mark\">A</div>"
    "Afrika<span>Integrated</span> <span class=\"acronym\">AFRIH</span></a>"
    "<p>A diversified Pan-African economic group mobilizing capital and developing "
    "enterprises across 12 sectors and 54 nations.</p>"
    "</div>"
    "<div class=\"footer-col\"><h4>Company</h4>"
    "<a href=\"/\">Home</a>"
    "<a href=\"/thesis\">Investment Thesis</a>"
    "<a href=\"/model\">Business Model</a>"
    "<a href=\"/sectors\">Sectors</a></div>"
    "<div class=\"footer-col\"><h4>Investors</h4>"
    "<a href=\"/invest\">Invest With Us</a>"
    "<a href=\"/governance\">Governance</a>"
    "<a href=\"/impact\">Impact &amp; ESG</a>"
    "<a href=\"/roadmap\">Roadmap</a></div>"
    "<div class=\"footer-col\"><h4>Engage</h4>"
    "<a href=\"/faq\">FAQ</a>"
    "<a href=\"/contact\">Contact</a>"
    "<a href=\"/invest\">Investor Inquiry</a></div>"
    "</div>"
    "<div class=\"footer-disclaimer\"><p>"
    "<strong style=\"color:var(--text2)\">Disclaimer:</strong> This document is a "
    "concept master plan for informational purposes only. It is not a legal, financial, "
    "or investment offering. Nothing herein constitutes an offer to sell or a solicitation "
    "of an offer to buy any security. All projections, targets, and forward-looking "
    "statements are illustrative and subject to change. AFRIH is in the concept formation stage."
    "</p></div>"
    "<div class=\"footer-bottom\"><p>&copy; 2026 Afrika Integrated Holdings (AFRIH). Concept Master Plan.</p>"
    "<p>Built with precision for Africa's future.</p></div>"
    "</div></footer>";

static const char *FAQ_JS =
    "<script>"
    "document.querySelectorAll('.faq-q').forEach(function(q){"
    "q.addEventListener('click',function(){this.parentElement.classList.toggle('open')});"
    "});"
    "document.querySelectorAll('form[action=\"/submit-investor\"],form[action=\"/submit-contact\"],"
    "form[action=\"/submit-newsletter\"]').forEach(function(f){"
    "f.addEventListener('submit',function(e){e.preventDefault();"
    "var btn=f.querySelector('button');var orig=btn.innerHTML;"
    "btn.innerHTML='Submitting...';btn.disabled=true;"
    "fetch(f.action,{method:'POST',body:new FormData(f)}).then(function(r){return r.json()})"
    ".then(function(r){btn.innerHTML=orig;btn.disabled=false;"
    "var alert=f.previousElementSibling;alert.className='alert alert-'+(r.success?'success':'error')+' show';"
    "alert.textContent=r.message||'Submitted';setTimeout(function(){alert.classList.remove('show')},6000);"
    "if(r.success)f.reset();}).catch(function(){btn.innerHTML=orig;btn.disabled=false});})});"
    "</script>";

/* ── Page wrapper ────────────────────────────────────────────── */
static char *page_wrap(const char *title, const char *body) {
    StrBuf sb; sb_init(&sb);
    sb_append(&sb, "<!DOCTYPE html><html lang=\"en\"><head>");
    sb_append(&sb, "<meta charset=\"UTF-8\">");
    sb_append(&sb, "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">");
    sb_appendf(&sb, "<title>%s</title>", title);
    sb_append(&sb, "<meta name=\"description\" content=\"AFRIH is a diversified Pan-African economic group mobilizing $1B+ in capital across 12 sectors and 54 African nations.\">");
    sb_append(&sb, "<meta property=\"og:title\" content=\"AFRIH \u2014 Afrika Integrated Holdings\">");
    sb_append(&sb, "<meta property=\"og:description\" content=\"A diversified Pan-African economic group mobilizing capital and developing enterprises across 12 sectors.\">");
    sb_append(&sb, "<meta property=\"og:image\" content=\"https://media.base44.com/images/public/6a13d9e8c07abf0ac4c23880/1dc96c134_generated_image.png\">");
    sb_append(&sb, "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">");
    sb_append(&sb, "<link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800;900&display=swap\" rel=\"stylesheet\">");
    sb_append(&sb, "<style>");
    sb_append(&sb, CSS);
    sb_append(&sb, "</style>");
    sb_append(&sb, "</head><body>");
    sb_append(&sb, NAV);
    sb_append(&sb, "<div class=\"page active\">");
    sb_append(&sb, body);
    sb_append(&sb, "</div>");
    sb_append(&sb, FOOTER);
    sb_append(&sb, FAQ_JS);
    sb_append(&sb, "</body></html>");
    return sb.data;
}

static char *page_home(void) {
    const char *body =
        "\n<div class=\"hero\">\n<div class=\"hero-bg\"><img src=\"https://media.base44.com/images/public/6a13d9e8c07abf0ac4c23880/1dc96c134_generated_image.png\" alt=\"African infrastructure and agriculture\"></div>\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Concept Master Plan · Seeking Strategic Capital Partners</div>\n<div class=\"tagline\">Pan-African Economic Group</div>\n<h1>Building Africa's<br><span>Integrated Future</span></h1>\n<p>An ecosystem platform that mobilizes capital, develops productive enterprises, and connects African value chains across 12 sectors and 54 nations.</p>\n<div class=\"hero-buttons\">\n<button class=\"btn btn-primary\" href=\"/invest\">Invest With Us →</button>\n<button class=\"btn btn-secondary\" href=\"/thesis\">Explore the Thesis</button>\n</div>\n</div>\n</div>\n\n<div class=\"stats-bar\">\n<div class=\"stats-bar-inner\">\n<div class=\"stat\"><div class=\"stat-num\">12</div><div class=\"stat-label\">Economic Sectors</div></div>\n<div class=\"stat\"><div class=\"stat-num\">54</div><div class=\"stat-label\">African Nations</div></div>\n<div class=\"stat\"><div class=\"stat-num\">$1B+</div><div class=\"stat-label\">Capital Vision</div></div>\n<div class=\"stat\"><div class=\"stat-num\">10</div><div class=\"stat-label\">Year Roadmap</div></div>\n<div class=\"stat\"><div class=\"stat-num\">5</div><div class=\"stat-label\">Capital Layers</div></div>\n</div>\n</div>\n\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">What We Do</span></div>\n<h2 class=\"section-title\">An Ecosystem <span>Orchestrator</span></h2>\n<p class=\"section-sub\">AFRIH doesn't just invest in projects — it builds the connective tissue between capital, enterprises, and markets across Africa.</p>\n<div class=\"grid grid-3\">\n<div class=\"card\"><div class=\"card-icon\">💰</div><h3>Capital Mobilization</h3><p>Source and deploy equity, debt, development finance, and project capital into bankable A"
        "frican opportunities across multiple risk-return profiles.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🏢</div><h3>Enterprise Development</h3><p>Build and operate productive businesses across agriculture, energy, industry, infrastructure, and technology — not just financial engineering.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🔗</div><h3>Value-Chain Integration</h3><p>Connect African markets by linking production, processing, logistics, distribution, and finance into integrated trade corridors.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🤝</div><h3>Strategic Partnerships</h3><p>Form joint ventures with governments, local companies, multinationals, DFIs, and financial institutions — sharing risk and amplifying impact.</p></div>\n<div class=\"card\"><div class=\"card-icon\">📊</div><h3>Governance & Risk</h3><p>Institutional-grade governance, risk management, and transparent reporting designed to earn and keep investor trust at every stage.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🌱</div><h3>Impact & ESG</h3><p>Measurable impact on jobs, local value creation, energy access, and inclusion — tracked and reported across all portfolio companies.</p></div>\n</div>\n</div>\n\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Architecture</span></div>\n<h2 class=\"section-title\">Group <span>Architecture</span></h2>\n<p class=\"section-sub\">A holding-company structure with specialized entities — each layer optimized for its role in the capital-and-enterprise stack.</p>\n<div class=\"table-wrap\"><table>\n<tr><th>Layer</th><th>Role</th><th>Capital Type</th></tr>\n<tr><td>Parent / Holding Company</td><td>Strategy, portfolio allocation, governance, treasury, investor relations</td><td><span class=\"badge badge-purple\">Strategic Equity</span></td></tr>\n<tr><td>Sector Platforms</td><td>Operating businesses and sector-specific subsidiaries</td><td><span class=\"badge badge-green\">Operating Eq"
        "uity</span></td></tr>\n<tr><td>Capital & Funds</td><td>Investment vehicles that raise and deploy capital into defined strategies</td><td><span class=\"badge badge-blue\">Fund Capital</span></td></tr>\n<tr><td>Country Platforms</td><td>Local operating and partnership structures</td><td><span class=\"badge badge-purple\">Local Equity + JV</span></td></tr>\n<tr><td>Project SPVs</td><td>Ring-fenced vehicles for major infrastructure and industrial projects</td><td><span class=\"badge badge-green\">Project Finance</span></td></tr>\n</table></div>\n</div>\n\n<div class=\"split\" style=\"max-width:1280px;margin:0 auto;padding:0 2rem 4rem;gap:3rem\">\n<div class=\"split-img\"><img src=\"https://media.base44.com/images/public/6a13d9e8c07abf0ac4c23880/93949ef8c_generated_image.png\" alt=\"Africa network visualization\"></div>\n<div class=\"split-content\">\n<div class=\"section-tag\">The Opportunity</div>\n<h2>Africa's moment is now</h2>\n<p>The world's youngest continent is urbanizing faster than any other. By 2050, 1 in 4 people on Earth will be African. The opportunities are not in resources alone — they are in building the infrastructure, industries, and institutions that 2.5 billion people will need.</p>\n<p>Yet capital, infrastructure, and markets remain fragmented. A group that connects these layers — matching the right capital to the right asset at the right stage — can create stronger economics than isolated projects ever could.</p>\n<button class=\"btn btn-primary\" href=\"/thesis\">Read the Investment Thesis →</button>\n</div>\n</div>\n\n<div class=\"cta-banner\">\n<h2>Ready to Partner?</h2>\n<p>We're seeking strategic capital partners, DFIs, institutional investors, and diaspora capital to build Africa's integrated future.</p>\n<div class=\"hero-buttons\">\n<button class=\"btn btn-primary\" href=\"/invest\">Submit Investor Inquiry</button>\n<button class=\"btn btn-secondary\" href=\"/contact\">Contact Us</button>\n</div>\n</div>\n\n<div class=\"newsletter-box\">\n<"
        "h3>Stay Updated on AFRIH</h3>\n<p>Receive updates on investment opportunities, project launches, and AFRIH milestones.</p>\n<div  class=\"alert\"></div>\n<form class=\"newsletter-form\"  action=\"/submit-newsletter\" method=\"POST\">\n<input type=\"email\" name=\"email\" placeholder=\"Your email address\" required>\n<button type=\"submit\" class=\"btn btn-green\" id=\"nl-btn-home\">Subscribe</button>\n</form>\n</div>\n</div>\n\n<!-- INVESTMENT THESIS"
    ;
    return page_wrap("AFRIH \u2014 Afrika Integrated Holdings | Pan-African Investment Group", body);
}

static char *page_thesis(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:60vh\">\n<div class=\"hero-bg\"><img src=\"https://media.base44.com/images/public/6a13d9e8c07abf0ac4c23880/93949ef8c_generated_image.png\" alt=\"\"></div>\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Investment Thesis</div>\n<h1>The <span>Thesis</span></h1>\n<p>Africa's economic opportunities are constrained not by absence of resources or entrepreneurs, but by fragmented capital, infrastructure, markets, and institutional execution.</p>\n</div>\n</div>\n\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Market Opportunity</span></div>\n<h2 class=\"section-title\">Why <span>Africa, Why Now</span></h2>\n<p class=\"section-sub\">The structural drivers are in place. What's missing is the connective platform.</p>\n<div class=\"grid grid-4\">\n<div class=\"feature-card\"><div class=\"num\">01</div><h3>Demographics</h3><p>1.4B people today, 2.5B by 2050. World's youngest population — median age 19.</p><ul><li>70% of population under 30</li><li>Rapid urbanization (4% annual)</li><li>Growing middle class</li></ul></div>\n<div class=\"feature-card\"><div class=\"num\">02</div><h3>Market Growth</h3><p>Africa's GDP projected to reach $15T by 2040. AfCFTA creates the world's largest free trade area.</p><ul><li>$3.4T combined GDP</li><li>1.3B-person single market</li><li>Intra-Africa trade growing</li></ul></div>\n<div class=\"feature-card\"><div class=\"num\">03</div><h3>Infrastructure Gap</h3><p>$100B+ annual infrastructure financing gap. Energy, transport, and industrial capacity are the binding constraints.</p><ul><li>600M without electricity</li><li>Low industrial value addition</li><li>Logistics costs 3x global average</li></ul></div>\n<div class=\"feature-card\"><div class=\"num\">04</div><h3>Capital Scarcity</h3><p>Private capital flows are a fraction of need. DFIs can't fill the gap alone — structured private capital is essential.</p><ul><li>$80B annual DF"
        "I flows</li><li>Minimal domestic institutional capital</li><li>Diaspora capital largely untapped</li></ul></div>\n</div>\n</div>\n\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Strategic Logic</span></div>\n<h2 class=\"section-title\">The <span>Integration Advantage</span></h2>\n<p class=\"section-sub\">Isolated projects capture isolated returns. Integrated platforms capture compounding returns.</p>\n<div class=\"grid grid-3\">\n<div class=\"card\"><div class=\"card-icon\">⚡</div><h3>Energy Powers Industry</h3><p>AFRIH energy projects supply power to AFRIH industrial operations — capturing margin internally and de-risking both sides.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🚚</div><h3>Logistics Connects Markets</h3><p>AFRIH logistics connects AFRIH agricultural output to AFRIH processing facilities to AFRIH distribution networks.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🏦</div><h3>Finance Fuels Growth</h3><p>AFRIH financial services provide working capital, trade finance, and insurance to AFRIH operating companies.</p></div>\n<div class=\"card\"><div class=\"card-icon\">💻</div><h3>Technology Coordinates</h3><p>AFRIH technology platform provides data, analytics, and coordination across the entire portfolio.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🌾</div><h3>Agriculture Feeds Cities</h3><p>AFRIH farms supply AFRIH processing — creating food security and exportable value-added products.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🔄</div><h3>Reinvestment Cycle</h3><p>Profits from operating businesses fund the next generation of growth — reducing external capital dependency over time.</p></div>\n</div>\n</div>\n\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Vision & Mission</span></div>\n<h2 class=\"section-title\">What Drives <span>AFRIH</span></h2>\n<div class=\"grid grid-2\">\n<div class=\"feature-card\"><h3>Vision</h3><p>Build a co"
        "nnected African economic platform that mobilizes capital and develops productive enterprises across the continent — creating lasting value for investors, communities, and nations.</p></div>\n<div class=\"feature-card\"><h3>Mission</h3><p>Create investable businesses and infrastructure, connect African markets, strengthen local value chains, and recycle economic value into further African growth — generation after generation.</p></div>\n</div>\n<div class=\"divider\"></div>\n<h2 class=\"section-title\">Core <span>Principles</span></h2>\n<p class=\"section-sub\">Six principles that govern every investment decision.</p>\n<div class=\"grid grid-3\">\n<div class=\"card\"><div class=\"card-icon\">🎯</div><h3>Execution Before Scale</h3><p>Prove a small number of businesses before expanding the portfolio. Results first, narrative second.</p></div>\n<div class=\"card\"><div class=\"card-icon\">⚖️</div><h3>Capital Matching</h3><p>Use equity, debt, grants, PPPs and project finance according to the cash-flow profile — never force one capital type.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🤝</div><h3>Local Partnership</h3><p>Enter countries through credible local operators, institutions, and community relationships. No parachute investing.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🔗</div><h3>Value-Chain Integration</h3><p>Prefer businesses that strengthen other businesses in the group. The sum must be greater than the parts.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🛡️</div><h3>Governance First</h3><p>Investor trust is an asset. Controls must grow ahead of complexity — always.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🔄</div><h3>Reinvestment</h3><p>Use a disciplined share of profits to fund the next generation of growth. Patient capital, compounding returns.</p></div>\n</div>\n</div>\n</div>\n\n<!-- BUSINESS MODEL"
    ;
    return page_wrap("Investment Thesis \u2014 AFRIH", body);
}

static char *page_model(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:50vh\">\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Business Model</div>\n<h1>How <span>AFRIH Creates Value</span></h1>\n<p>A layered capital model matching maturity of capital to maturity of asset — with multiple revenue streams across the platform.</p>\n</div>\n</div>\n\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Capital Architecture</span></div>\n<h2 class=\"section-title\">Layered <span>Capital Model</span></h2>\n<p class=\"section-sub\">Each layer serves a specific purpose — from high-risk formation capital to mature-market bonds.</p>\n<div class=\"table-wrap\"><table>\n<tr><th>Capital Type</th><th>Typical Use</th><th>Risk Profile</th><th>Target Return</th></tr>\n<tr><td>Founder / Strategic Equity</td><td>Formation, early pilots, acquisitions</td><td>High</td><td>25-40%+ IRR</td></tr>\n<tr><td>Institutional Equity</td><td>Scale and growth phase</td><td>Medium-High</td><td>18-25% IRR</td></tr>\n<tr><td>DFI Capital</td><td>Development and infrastructure</td><td>Medium</td><td>8-15% IRR</td></tr>\n<tr><td>Commercial Debt</td><td>Working capital, equipment, expansion</td><td>Medium</td><td>12-18% interest</td></tr>\n<tr><td>Project Finance</td><td>Large ring-fenced infrastructure assets</td><td>Project-specific</td><td>10-18% project IRR</td></tr>\n<tr><td>Bonds / Capital Markets</td><td>Mature expansion and refinancing</td><td>Market-dependent</td><td>Market rate</td></tr>\n<tr><td>Retained Earnings</td><td>Reinvestment in next-generation growth</td><td>Lower</td><td>Compounding</td></tr>\n</table></div>\n</div>\n\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Revenue Streams</span></div>\n<h2 class=\"section-title\">Multiple <span>Revenue Engines</span></h2>\n<div class=\"grid grid-3\">\n<div class=\"feature-card\"><div class=\"num\">01</div><h3>Operating Revenue</h3><p>Revenue from portfolio companies "
        "— agriculture output, energy sales, manufacturing, logistics services, property rental, healthcare fees, education tuition.</p></div>\n<div class=\"feature-card\"><div class=\"num\">02</div><h3>Capital Gains</h3><p>Equity appreciation in portfolio companies and projects. Exit multiples on mature investments via trade sales, IPOs, or secondary buyouts.</p></div>\n<div class=\"feature-card\"><div class=\"num\">03</div><h3>Fund Management Fees</h3><p>Management fees (1.5-2.5%) and carried interest (15-25%) on dedicated investment vehicles and sector funds.</p></div>\n<div class=\"feature-card\"><div class=\"num\">04</div><h3>Financial Services</h3><p>Interest income, trade finance margins, insurance premiums, and fintech transaction fees from financial sector platforms.</p></div>\n<div class=\"feature-card\"><div class=\"num\">05</div><h3>Strategic Dividends</h3><p>Dividend income from mature portfolio companies — recycled into new investments per the reinvestment principle.</p></div>\n<div class=\"feature-card\"><div class=\"num\">06</div><h3>Advisory & Structuring</h3><p>Project structuring fees, PPP advisory, and capital arrangement services for external clients and partners.</p></div>\n</div>\n</div>\n\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Fund Structure</span></div>\n<h2 class=\"section-title\">Investment <span>Vehicles</span></h2>\n<p class=\"section-sub\">Multiple entry points for different investor profiles — from $10K diaspora tickets to $250M institutional commitments.</p>\n<div class=\"grid grid-2\">\n<div class=\"feature-card\"><span class=\"badge badge-purple\" style=\"margin-bottom:1rem;display:inline-block\">Growth Fund</span><h3>$10M–$50M Tickets</h3><p>Acquire and scale established companies across high-growth African sectors. Target IRR: 18-25%. 7-year fund life with 2-year extension option.</p><ul><li>Sector agnostic within AFRIH focus areas</li><li>Majority or significant minority stakes</li><li>Active"
        " governance participation</li></ul></div>\n<div class=\"feature-card\"><span class=\"badge badge-green\" style=\"margin-bottom:1rem;display:inline-block\">Infrastructure Fund</span><h3>$50M–$250M</h3><p>Energy, transport, industrial zones, and water infrastructure. Target IRR: 12-18%. 10-12 year fund life.</p><ul><li>Project finance structuring</li><li>DFI co-investment</li><li>Government/PPP partnerships</li></ul></div>\n<div class=\"feature-card\"><span class=\"badge badge-blue\" style=\"margin-bottom:1rem;display:inline-block\">Venture Fund</span><h3>$1M–$10M Tickets</h3><p>Technology and startups driving digital transformation. Target IRR: 25-40%+. 8-year fund life.</p><ul><li>Seed through Series B</li><li>Africa-focused tech</li><li>Follow-on reserves</li></ul></div>\n<div class=\"feature-card\"><span class=\"badge badge-purple\" style=\"margin-bottom:1rem;display:inline-block\">Diaspora Platform</span><h3>From $10K</h3><p>Channel diaspora capital into productive African assets with transparent reporting and fractional participation.</p><ul><li>Accessible minimum tickets</li><li>Quarterly impact reporting</li><li>Digital onboarding</li></ul></div>\n</div>\n</div>\n</div>\n\n<!-- SECTORS"
    ;
    return page_wrap("Business Model \u2014 AFRIH", body);
}

static char *page_sectors(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:50vh\">\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Sector Portfolio</div>\n<h1>12 <span>Economic Sectors</span></h1>\n<p>Building Africa's productive capacity across the full economic stack.</p>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"grid grid-4\">\n<div class=\"card\"><div class=\"card-icon\">🌾</div><h3>Agriculture & Food</h3><p>Farming, livestock, fisheries, storage, processing, exports. Food security and value addition.</p><span class=\"badge badge-green\">Foundational</span></div>\n<div class=\"card\"><div class=\"card-icon\">⚡</div><h3>Energy</h3><p>Solar, hydro, transmission, distributed energy. Power enables every other sector.</p><span class=\"badge badge-purple\">Enabling</span></div>\n<div class=\"card\"><div class=\"card-icon\">🏭</div><h3>Industry</h3><p>Manufacturing, materials, machinery, consumer goods. Captures value locally.</p><span class=\"badge badge-green\">Productive</span></div>\n<div class=\"card\"><div class=\"card-icon\">🚧</div><h3>Infrastructure</h3><p>Roads, rail, ports, airports, water, industrial zones. Enables trade.</p><span class=\"badge badge-blue\">Enabling</span></div>\n<div class=\"card\"><div class=\"card-icon\">🚚</div><h3>Logistics & Trade</h3><p>Warehousing, trucking, shipping, distribution, e-commerce. Connects producers to markets.</p><span class=\"badge badge-green\">Connective</span></div>\n<div class=\"card\"><div class=\"card-icon\">🏦</div><h3>Finance</h3><p>Investment, fintech, insurance, trade finance. Mobilizes and allocates capital.</p><span class=\"badge badge-purple\">Cross-cutting</span></div>\n<div class=\"card\"><div class=\"card-icon\">💻</div><h3>Technology</h3><p>Software, data, AI, telecom, digital platforms. Improves productivity and coordination.</p><span class=\"badge badge-blue\">Cross-cutting</span></div>\n<div class=\"card\"><div class=\"card-icon\">🏥</div><h3>Healthcare</h3><p>Hospitals, diagnostics, pharm"
        "aceuticals, equipment. Human-capital resilience.</p><span class=\"badge badge-green\">Social</span></div>\n<div class=\"card\"><div class=\"card-icon\">🎓</div><h3>Education</h3><p>Schools, vocational training, universities, research. Builds the talent pipeline.</p><span class=\"badge badge-green\">Social</span></div>\n<div class=\"card\"><div class=\"card-icon\">🏠</div><h3>Housing & Cities</h3><p>Residential, commercial, urban infrastructure. Supports urbanization.</p><span class=\"badge badge-purple\">Productive</span></div>\n<div class=\"card\"><div class=\"card-icon\">✈️</div><h3>Tourism & Services</h3><p>Hotels, travel, entertainment, cultural economy. Foreign exchange and jobs.</p><span class=\"badge badge-green\">Revenue</span></div>\n<div class=\"card\"><div class=\"card-icon\">📋</div><h3>Cross-Sector</h3><p>Value-chain integration linking production, processing, logistics, finance, and technology.</p><span class=\"badge badge-purple\">Integration</span></div>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Capital Stack</span></div>\n<h2 class=\"section-title\">How Capital <span>Flows Through Sectors</span></h2>\n<p class=\"section-sub\">Each sector attracts the capital type best suited to its risk-return and cash-flow profile.</p>\n<div class=\"table-wrap\"><table>\n<tr><th>Sector</th><th>Primary Capital</th><th>Typical Ticket</th><th>Horizon</th></tr>\n<tr><td>Agriculture & Food</td><td>Equity + DFI + Trade Finance</td><td>$5M–$50M</td><td>5–10 years</td></tr>\n<tr><td>Energy</td><td>Project Finance + DFI</td><td>$50M–$500M</td><td>10–20 years</td></tr>\n<tr><td>Industry</td><td>Equity + Debt</td><td>$10M–$100M</td><td>5–10 years</td></tr>\n<tr><td>Infrastructure</td><td>Project Finance + PPP</td><td>$100M–$1B+</td><td>15–25 years</td></tr>\n<tr><td>Logistics & Trade</td><td>Equity + Working Capital</td><td>$5M–$50M</td><td>5–8 years</td></tr>\n<tr><td>Finance</td><td>Equity + Regulatory Capital</td><td"
        ">$10M–$100M</td><td>Ongoing</td></tr>\n<tr><td>Technology</td><td>Venture Equity</td><td>$0.5M–$10M</td><td>5–8 years</td></tr>\n<tr><td>Healthcare</td><td>Equity + DFI</td><td>$5M–$50M</td><td>7–12 years</td></tr>\n<tr><td>Education</td><td>Equity + Grants</td><td>$2M–$20M</td><td>7–10 years</td></tr>\n<tr><td>Housing & Cities</td><td>Equity + Debt + Project Finance</td><td>$10M–$200M</td><td>7–15 years</td></tr>\n<tr><td>Tourism & Services</td><td>Equity</td><td>$5M–$30M</td><td>5–8 years</td></tr>\n</table></div>\n</div>\n</div>\n\n<!-- GOVERNANCE"
    ;
    return page_wrap("Sectors \u2014 AFRIH", body);
}

static char *page_governance(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:50vh\">\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Governance & Risk</div>\n<h1>Trust Is a <span>Financing Asset</span></h1>\n<p>Governance is designed before the organization becomes large — not after.</p>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Governance Framework</span></div>\n<h2 class=\"section-title\">Governance <span>Bodies</span></h2>\n<p class=\"section-sub\">Multiple oversight bodies ensure accountability at every level of the organization.</p>\n<div class=\"grid grid-3\">\n<div class=\"card\"><div class=\"card-icon\">🏛️</div><h3>Board of Directors</h3><p>Strategy, oversight, CEO appointment, major transaction approval. Independent directors required above $100M AUM.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🔍</div><h3>Audit & Risk Committee</h3><p>Financial reporting, internal controls, risk management, external audit oversight. Reports directly to board.</p></div>\n<div class=\"card\"><div class=\"card-icon\">📈</div><h3>Investment Committee</h3><p>Capital allocation, investment approvals, portfolio review, exit decisions. Includes external industry experts.</p></div>\n<div class=\"card\"><div class=\"card-icon\">💰</div><h3>Credit / Finance Committee</h3><p>Debt approvals, guarantees, liquidity management, treasury policy. Independent risk officer has veto authority.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🌱</div><h3>ESG / Impact Committee</h3><p>Environmental, social, and impact standards. Screens investments, monitors portfolio ESG performance, publishes annual impact report.</p></div>\n<div class=\"card\"><div class=\"card-icon\">🛡️</div><h3>Internal Audit</h3><p>Independent control testing across all entities. Reports to Audit Committee, not management. Full operational independence.</p></div>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class="
        "\"section-tag\">Risk Management</span></div>\n<h2 class=\"section-title\">Risk <span>Framework</span></h2>\n<p class=\"section-sub\">Structured risk identification, measurement, mitigation, and monitoring across the platform.</p>\n<div class=\"table-wrap\"><table>\n<tr><th>Risk Category</th><th>Mitigation Strategy</th></tr>\n<tr><td>Political / Country Risk</td><td>Diversification across countries, local partnerships, political risk insurance (MIGA/ATI), PPP structures</td></tr>\n<tr><td>Currency Risk</td><td>Revenue-currency matching, hedging, local-currency financing where available, natural hedges via local sourcing</td></tr>\n<tr><td>Execution Risk</td><td>Phase-based capital deployment, milestone gates, experienced operating partners, stage-gate reviews</td></tr>\n<tr><td>Market / Demand Risk</td><td>Diversified sector exposure, demand validation before scaling, contracted revenue where possible</td></tr>\n<tr><td>Regulatory Risk</td><td>Government partnerships, regulatory compliance teams, sector-specific legal counsel, early engagement</td></tr>\n<tr><td>Financial / Liquidity</td><td>Conservative leverage, liquidity reserves, staged capital calls, diversification of funding sources</td></tr>\n<tr><td>ESG / Reputational</td><td>ESG screening pre-investment, impact monitoring, community engagement, transparent reporting</td></tr>\n</table></div>\n</div>\n</div>\n\n<!-- IMPACT"
    ;
    return page_wrap("Governance & Risk \u2014 AFRIH", body);
}

static char *page_impact(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:50vh\">\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Impact & ESG</div>\n<h1>Measurable <span>Impact</span></h1>\n<p>Financial returns and social impact are not competing priorities — they are compounding ones.</p>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Impact Pillars</span></div>\n<h2 class=\"section-title\">What We <span>Measure</span></h2>\n<div class=\"grid grid-4\">\n<div class=\"feature-card\"><div class=\"card-icon\" style=\"margin-bottom:1rem\">💼</div><h3>Jobs Created</h3><p>Direct and indirect employment across portfolio companies. Target: 50,000+ jobs by Year 10.</p></div>\n<div class=\"feature-card\"><div class=\"card-icon\" style=\"margin-bottom:1rem\">⚡</div><h3>Energy Access</h3><p>Megawatts of clean energy deployed. People gaining reliable electricity access. Target: 500MW+ by Year 10.</p></div>\n<div class=\"feature-card\"><div class=\"card-icon\" style=\"margin-bottom:1rem\">🌍</div><h3>Local Value Addition</h3><p>Processing capacity built locally. Import substitution achieved. Export revenue generated. Target: $500M+ annual local value.</p></div>\n<div class=\"feature-card\"><div class=\"card-icon\" style=\"margin-bottom:1rem\">📊</div><h3>Capital Mobilized</h3><p>Total capital deployed into African productive assets. DFI co-investment leveraged. Target: $1B+ by Year 10.</p></div>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">ESG Standards</span></div>\n<h2 class=\"section-title\">ESG <span>Framework</span></h2>\n<div class=\"grid grid-3\">\n<div class=\"card\"><div class=\"card-icon\">🌍</div><h3>Environmental</h3><p>Carbon footprint tracking, renewable energy preference, water stewardship, biodiversity protection, waste management standards across all operations.</p></div>\n<div class=\"card\"><div class=\"card-icon\">👥</div><h3>Social</h3><p>Fair labor"
        " practices, gender inclusion targets (40% minimum), community engagement protocols, local content policies, health & safety standards.</p></div>\n<div class=\"card\"><div class=\"card-icon\">📋</div><h3>Governance</h3><p>Anti-corruption policies, transparent procurement, board independence, whistleblower protections, third-party ethics audits.</p></div>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Reporting</span></div>\n<h2 class=\"section-title\">Transparent <span>Reporting</span></h2>\n<p class=\"section-sub\">Investors and stakeholders receive regular, standardized impact reporting.</p>\n<div class=\"grid grid-4\">\n<div class=\"card\"><h3>Quarterly</h3><p>Operational and financial KPIs across portfolio companies.</p></div>\n<div class=\"card\"><h3>Annual</h3><p>Full impact report aligned with IFC Performance Standards and IRIS+ metrics.</p></div>\n<div class=\"card\"><h3>Biennial</h3><p>Independent ESG audit and third-party impact verification.</p></div>\n<div class=\"card\"><h3>Ad Hoc</h3><p>Material event reporting for significant developments or incidents.</p></div>\n</div>\n</div>\n</div>\n\n<!-- ROADMAP"
    ;
    return page_wrap("Impact & ESG \u2014 AFRIH", body);
}

static char *page_roadmap(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:50vh\">\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> 10-Year Roadmap</div>\n<h1>Milestone-Driven <span>Growth</span></h1>\n<p>Prove, Institutionalize, Scale, Recycle Capital — phase by phase.</p>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Strategic Timeline</span></div>\n<div class=\"timeline\">\n<div class=\"timeline-item\"><div class=\"year\">Year 1</div><h4>Formation + Pilots</h4><p>Establish parent structure, governance framework, and first capital commitments. Launch 3-5 pilot projects in priority sectors.</p><div class=\"metrics\"><span class=\"metric\">$1M–$10M raised</span><span class=\"metric\">3-5 pilots</span><span class=\"metric\">2 sectors</span></div></div>\n<div class=\"timeline-item\"><div class=\"year\">Years 2-3</div><h4>Proof + Fundraising Engine</h4><p>Generate operating revenue. Build investor pipeline and repeatable project model. Establish dedicated sector subsidiaries.</p><div class=\"metrics\"><span class=\"metric\">$10M–$50M raised</span><span class=\"metric\">5-10 companies</span><span class=\"metric\">4 sectors</span></div></div>\n<div class=\"timeline-item\"><div class=\"year\">Years 3-5</div><h4>Sector Expansion</h4><p>Multiple subsidiaries operating. JV pipeline active. DFI relationships established. Project-finance templates deployed.</p><div class=\"metrics\"><span class=\"metric\">$50M–$150M raised</span><span class=\"metric\">15-25 companies</span><span class=\"metric\">6+ sectors</span></div></div>\n<div class=\"timeline-item\"><div class=\"year\">Years 5-7</div><h4>Regional Scale</h4><p>Country platforms launched. Larger funds raised. Project-finance capability in-house. Cross-border trade corridors active.</p><div class=\"metrics\"><span class=\"metric\">$150M–$500M raised</span><span class=\"metric\">30+ companies</span><span class=\"metric\">8+ countries</span></div></div>\n<div class=\"ti"
        "meline-item\"><div class=\"year\">Years 7-10</div><h4>Institutional Scale</h4><p>Capital markets access. Major infrastructure portfolio. Mature governance with independent directors. Capital recycling in full effect.</p><div class=\"metrics\"><span class=\"metric\">$500M–$1B+ raised</span><span class=\"metric\">50+ companies</span><span class=\"metric\">10+ countries</span></div></div>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Fundraising Targets</span></div>\n<h2 class=\"section-title\">Capital <span>Raising Plan</span></h2>\n<div class=\"table-wrap\"><table>\n<tr><th>Stage</th><th>Capital Objective</th><th>Primary Sources</th><th>Use</th></tr>\n<tr><td>Formation</td><td>$1M–$10M</td><td>Founders, strategic investors</td><td>Structure, governance, pilots</td></tr>\n<tr><td>Early Scale</td><td>$10M–$50M</td><td>Family offices, diaspora, strategic capital</td><td>Operating companies, first JVs</td></tr>\n<tr><td>Institutional</td><td>$50M–$250M</td><td>Institutions, DFIs, PE funds</td><td>Sector expansion, country platforms</td></tr>\n<tr><td>Large Projects</td><td>$100M–$1B+ per project</td><td>Project finance, JV, DFI, banks</td><td>Infrastructure, energy, industrial</td></tr>\n<tr><td>Mature Markets</td><td>Case-specific</td><td>Bonds, public markets, funds</td><td>Refinancing, expansion, exits</td></tr>\n</table></div>\n</div>\n</div>\n\n<!-- FAQ"
    ;
    return page_wrap("Roadmap \u2014 AFRIH", body);
}

static char *page_faq(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:40vh\">\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Investor FAQ</div>\n<h1>Questions <span>& Answers</span></h1>\n<p>Everything investors need to know before engaging with AFRIH.</p>\n</div>\n</div>\n<div class=\"section\" style=\"max-width:850px\">\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">What stage is AFRIH at currently? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>AFRIH is at the concept master plan stage. The strategic framework, business model, governance design, and 10-year roadmap are complete. We are now seeking formation capital and strategic partners to begin execution in Year 1.</p></div></div>\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">What is the minimum investment? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>Minimum tickets vary by vehicle: $10K for the diaspora platform, $1M for the venture fund, $10M for the growth fund, and $50M+ for the infrastructure fund. Strategic partners and JVs are handled case by case.</p></div></div>\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">What returns do you target? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>Returns vary by capital layer: 25-40%+ IRR for venture/strategic equity, 18-25% for growth equity, 12-18% for infrastructure/project finance, and 8-15% for DFI-aligned capital. The blended platform target is 20%+ net IRR over a 10-year horizon.</p></div></div>\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">How do you manage political risk? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>Through country diversification, local partnerships with credible operators, political risk insurance (MIGA/ATI), PPP structures, and phased capital deployment wi"
        "th milestone gates. No single country exposure exceeds 25% of portfolio at maturity.</p></div></div>\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">What is the exit strategy? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>Exits include trade sales to strategic buyers, secondary buyouts, IPOs on African exchanges, and recapitalizations. Infrastructure assets may be held long-term with refinancing events. The reinvestment principle means a disciplined share of exit proceeds funds new growth.</p></div></div>\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">How is AFRIH different from other Africa funds? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>AFRIH is not just a fund — it's an operating platform. We build and operate businesses, not just financial positions. The integration advantage means our companies strengthen each other, creating compounding returns that isolated investments cannot achieve.</p></div></div>\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">What reporting do investors receive? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>Quarterly operational and financial KPIs, annual full impact reports aligned with IFC Performance Standards and IRIS+ metrics, biennial independent ESG audits, and ad-hoc material event reporting. All reporting is standardized and investor-accessible.</p></div></div>\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">Is AFRIH regulated? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>AFRIH will be structured under appropriate regulatory frameworks in its domicile jurisdiction. Fund vehicles will be regulated per their operating jurisdiction. This is a concept master plan — regulatory structure will be finalized during formation with legal counsel.</p></div></div>\n<d"
        "iv class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">How can diaspora investors participate? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>The diaspora platform offers accessible minimum tickets from $10K with digital onboarding, quarterly impact reporting, and fractional participation in productive African assets. It's designed for Africans abroad who want to invest back home with transparency.</p></div></div>\n<div class=\"faq-item\"><div class=\"faq-q\" onclick=\"this.parentElement.classList.toggle('open')\">What is the governance structure? <span class=\"arrow\">+</span></div><div class=\"faq-a\"><p>A board with independent directors (required above $100M AUM), plus five standing committees: Audit & Risk, Investment, Credit/Finance, ESG/Impact, and Internal Audit. Full operational independence for risk functions.</p></div></div>\n</div>\n</div>\n\n<!-- INVEST"
    ;
    return page_wrap("FAQ \u2014 AFRIH", body);
}

static char *page_invest(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:50vh\">\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Investor Onboarding</div>\n<h1>Invest With <span>AFRIH</span></h1>\n<p>Partner with us to build Africa's economic future. Tell us about your interests and our capital team will respond within 5 business days.</p>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Investment Vehicles</span></div>\n<h2 class=\"section-title\">Choose Your <span>Entry Point</span></h2>\n<div class=\"grid grid-3\">\n<div class=\"card\"><div class=\"card-icon\">📈</div><h3>Growth Fund</h3><p>$10M–$50M tickets. Acquire and scale established companies. Target 18-25% IRR.</p><span class=\"badge badge-purple\">7-year fund</span></div>\n<div class=\"card\"><div class=\"card-icon\">🏗️</div><h3>Infrastructure Fund</h3><p>$50M–$250M. Energy, transport, industrial zones. Target 12-18% IRR.</p><span class=\"badge badge-green\">10-12 year fund</span></div>\n<div class=\"card\"><div class=\"card-icon\">🚀</div><h3>Venture Fund</h3><p>$1M–$10M tickets. Tech and digital transformation. Target 25-40%+ IRR.</p><span class=\"badge badge-blue\">8-year fund</span></div>\n<div class=\"card\"><div class=\"card-icon\">🌍</div><h3>Diaspora Platform</h3><p>From $10K. Accessible investment in productive African assets.</p><span class=\"badge badge-purple\">Open access</span></div>\n<div class=\"card\"><div class=\"card-icon\">🤝</div><h3>Joint Ventures</h3><p>Co-invest in specific projects with local and strategic partners.</p><span class=\"badge badge-green\">Case by case</span></div>\n<div class=\"card\"><div class=\"card-icon\">💚</div><h3>Impact Vehicle</h3><p>Non-commercial/catalytic programs with foundations and donors.</p><span class=\"badge badge-blue\">Grant-aligned</span></div>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"section-tag-wrap\"><span class=\"section-tag\">Inquiry Form</span></div>\n<h2 class=\"section-ti"
        "tle\">Submit Investor <span>Inquiry</span></h2>\n<p class=\"section-sub\">All inquiries are confidential. Our capital team responds within 5 business days.</p>\n<div id=\"investor-alert\" class=\"alert\"></div>\n<form id=\"investor-form\" action=\"/submit-investor\" method=\"POST\" style=\"max-width:750px;margin:0 auto\">\n<div class=\"form-row\">\n<div class=\"form-group\"><label>Full Name *</label><input type=\"text\" name=\"name\" required></div>\n<div class=\"form-group\"><label>Email *</label><input type=\"email\" name=\"email\" required></div>\n</div>\n<div class=\"form-row\">\n<div class=\"form-group\"><label>Organization</label><input type=\"text\" name=\"organization\"></div>\n<div class=\"form-group\"><label>Investor Type *</label><select name=\"investorType\" required><option value=\"\">Select type</option><option>Family Office</option><option>Pension Fund</option><option>DFI</option><option>Corporate Partner</option><option>Diaspora Investor</option><option>Commercial Bank</option><option>Government/PPP</option><option>Strategic Investor</option></select></div>\n</div>\n<div class=\"form-row\">\n<div class=\"form-group\"><label>Ticket Size *</label><select name=\"ticketSize\" required><option value=\"\">Select range</option><option>$1M-$10M</option><option>$10M-$50M</option><option>$50M-$250M</option><option>$100M-$1B+</option><option>To discuss</option></select></div>\n<div class=\"form-group\"><label>Sector Interest</label><input type=\"text\" name=\"sectorInterest\" placeholder=\"e.g. Energy, Agriculture, Technology\"></div>\n</div>\n<div class=\"form-group\"><label>Country Interest</label><input type=\"text\" name=\"countryInterest\" placeholder=\"e.g. Uganda, Kenya, Nigeria\"></div>\n<div class=\"form-group\"><label>Message *</label><textarea name=\"message\" required placeholder=\"Tell us about your investment objectives and how you'd like to partner with AFRIH.\"></textarea></div>\n<button type=\"submit\" class=\"btn btn-primary\" style=\"width:10"
        "0%\" id=\"investor-btn\">Submit Inquiry →</button>\n</form>\n</div>\n</div>\n\n<!-- CONTACT"
    ;
    return page_wrap("Invest \u2014 AFRIH", body);
}

static char *page_contact(void) {
    const char *body =
        "\n<div class=\"hero\" style=\"min-height:40vh\">\n<div class=\"hero-content\">\n<div class=\"hero-badge\"><span class=\"dot\"></span> Contact Us</div>\n<h1>Get <span>In Touch</span></h1>\n<p>Partnership inquiries, media requests, or general questions — we'd love to hear from you.</p>\n</div>\n</div>\n<div class=\"section\">\n<div class=\"contact-grid\">\n<div class=\"contact-info-card\">\n<h3>AFRIH at a Glance</h3>\n<div class=\"info-row\"><div class=\"info-icon\">🏛️</div><div><div class=\"info-label\">Organization</div><div class=\"info-value\">Afrika Integrated Holdings (AFRIH)</div></div></div>\n<div class=\"info-row\"><div class=\"info-icon\">📍</div><div><div class=\"info-label\">Geography</div><div class=\"info-value\">Pan-African, multi-country platform</div></div></div>\n<div class=\"info-row\"><div class=\"info-icon\">💼</div><div><div class=\"info-label\">Sectors</div><div class=\"info-value\">12 economic sectors</div></div></div>\n<div class=\"info-row\"><div class=\"info-icon\">💰</div><div><div class=\"info-label\">Capital Vision</div><div class=\"info-value\">$1B+ over 10 years</div></div></div>\n<div class=\"info-row\"><div class=\"info-icon\">🤝</div><div><div class=\"info-label\">Partnership Types</div><div class=\"info-value\">Strategic investors, DFIs, banks, diaspora, government/PPP</div></div></div>\n<div class=\"info-row\"><div class=\"info-icon\">📋</div><div><div class=\"info-label\">Status</div><div class=\"info-value\">Concept Master Plan — seeking formation partners</div></div></div>\n</div>\n<div>\n<h2 class=\"section-title\" style=\"text-align:left;font-size:1.5rem;margin-bottom:1.5rem\">Send a <span style=\"color:var(--purple)\">Message</span></h2>\n<div id=\"contact-alert\" class=\"alert\"></div>\n<form id=\"contact-form\" action=\"/submit-contact\" method=\"POST\">\n<div class=\"form-group\"><label>Full Name *</label><input type=\"text\" name=\"name\" required></div>\n<div class=\"form-group\"><label>Email *</label><input type=\"email\" na"
        "me=\"email\" required></div>\n<div class=\"form-group\"><label>Organization</label><input type=\"text\" name=\"organization\"></div>\n<div class=\"form-group\"><label>Interest Type *</label><select name=\"interestType\" required><option value=\"\">Select interest</option><option>Investor</option><option>DFI</option><option>JV Partner</option><option>Government/PPP</option><option>Diaspora</option><option>Corporate</option><option>General</option></select></div>\n<div class=\"form-group\"><label>Message *</label><textarea name=\"message\" required placeholder=\"How can we help you?\"></textarea></div>\n<button type=\"submit\" class=\"btn btn-primary\" style=\"width:100%\" id=\"contact-btn\">Send Message →</button>\n</form>\n</div>\n</div>\n</div>\n<div class=\"newsletter-box\">\n<h3>Stay Updated on AFRIH</h3>\n<p>Subscribe to receive updates on investment opportunities, project launches, and AFRIH news.</p>\n<div  class=\"alert\"></div>\n<form class=\"newsletter-form\"  action=\"/submit-newsletter\" method=\"POST\">\n<input type=\"email\" name=\"email\" placeholder=\"Your email address\" required>\n<button type=\"submit\" class=\"btn btn-green\" id=\"nl-btn-contact\">Subscribe</button>\n</form>\n</div>\n"
    ;
    return page_wrap("Contact \u2014 AFRIH", body);
}

/* ── Form handler ────────────────────────────────────────────── */

/* ── 404 ─────────────────────────────────────────────────────── */
static char *page_404(void) {
    StrBuf sb; sb_init(&sb);
    sb_append(&sb, "<!DOCTYPE html><html lang=\"en\"><head>");
    sb_append(&sb, "<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">");
    sb_append(&sb, "<title>404 \u2014 AFRIH</title>");
    sb_append(&sb, "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">");
    sb_append(&sb, "<link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800;900&display=swap\" rel=\"stylesheet\">");
    sb_append(&sb, "<style>");
    sb_append(&sb, CSS);
    sb_append(&sb, "</style></head><body>");
    sb_append(&sb, NAV);
    sb_append(&sb, "<div class=\"page active\" style=\"min-height:80vh;display:flex;align-items:center;justify-content:center;text-align:center\">");
    sb_append(&sb, "<div><h1 style=\"font-size:4rem;color:var(--purple)\">404</h1>");
    sb_append(&sb, "<p style=\"color:var(--text2);margin-bottom:2rem\">This page doesn't exist.</p>");
    sb_append(&sb, "<a href=\"/\" class=\"btn btn-primary\">Go Home</a></div>");
    sb_append(&sb, "</div>");
    sb_append(&sb, FOOTER);
    sb_append(&sb, "</body></html>");
    return sb.data;
}

/* ── HTTP helpers ────────────────────────────────────────────── */
static void send_response(int fd, int status, const char *ctype,
                          const char *body, size_t len) {
    char header[512];
    const char *st = (status == 200) ? "OK" : (status == 404) ? "Not Found" : "OK";
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Server: afrih-c/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Cache-Control: public, max-age=3600\r\n"
        "X-Powered-By: Pure C\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, st, ctype, len);
    write(fd, header, hlen);
    if (body && len > 0) write(fd, body, len);
}

static void send_html(int fd, char *html) {
    send_response(fd, 200, "text/html; charset=utf-8", html, strlen(html));
    free(html);
}

static void send_json(int fd, int success, const char *message) {
    char json[512];
    int len = snprintf(json, sizeof(json),
        "{\"success\":%s,\"message\":\"%s\"}",
        success ? "true" : "false", message);
    send_response(fd, 200, "application/json", json, len);
}

/* ── Static files ────────────────────────────────────────────── */
static const char *get_mime(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (!strcmp(dot, ".webp")) return "image/webp";
    if (!strcmp(dot, ".jpg") || !strcmp(dot, ".jpeg")) return "image/jpeg";
    if (!strcmp(dot, ".png")) return "image/png";
    if (!strcmp(dot, ".svg")) return "image/svg+xml";
    if (!strcmp(dot, ".css")) return "text/css";
    if (!strcmp(dot, ".js")) return "application/javascript";
    return "application/octet-stream";
}

static void serve_static(int fd, const char *path) {
    char fpath[4096];
    snprintf(fpath, sizeof(fpath), "public%s", path);
    FILE *f = fopen(fpath, "rb");
    if (!f) { char *p = page_404(); send_html(fd, p); return; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz); if (sz > 0) fread(buf, 1, sz, f); fclose(f);
    send_response(fd, 200, get_mime(path), buf, sz);
    free(buf);
}

/* ── Parse POST body ─────────────────────────────────────────── */

/* ── Request handler ─────────────────────────────────────────── */
static void *handle_client(void *arg) {
    int fd = *(int*)arg;
    free(arg);
    char req[MAXREQ + 1];
    ssize_t n = recv(fd, req, MAXREQ, 0);
    if (n <= 0) { close(fd); return NULL; }
    req[n] = '\0';
    
    char method[8], path[MAXPATH];
    if (sscanf(req, "%7s %2047s", method, path) < 2) {
        char *p = page_404(); send_html(fd, p); close(fd); return NULL;
    }
    
    /* Strip query string */
    char *q = strchr(path, '?');
    if (q) *q = '\0';
    
    /* GET routes */
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0)
            send_html(fd, page_home());
        else if (strcmp(path, "/thesis") == 0) send_html(fd, page_thesis());
        else if (strcmp(path, "/model") == 0) send_html(fd, page_model());
        else if (strcmp(path, "/sectors") == 0) send_html(fd, page_sectors());
        else if (strcmp(path, "/governance") == 0) send_html(fd, page_governance());
        else if (strcmp(path, "/impact") == 0) send_html(fd, page_impact());
        else if (strcmp(path, "/roadmap") == 0) send_html(fd, page_roadmap());
        else if (strcmp(path, "/faq") == 0) send_html(fd, page_faq());
        else if (strcmp(path, "/invest") == 0) send_html(fd, page_invest());
        else if (strcmp(path, "/contact") == 0) send_html(fd, page_contact());
        else if (strcmp(path, "/robots.txt") == 0) {
            const char *r = "User-agent: *\nAllow: /\nSitemap: https://afrih.example/sitemap.xml\n";
            send_response(fd, 200, "text/plain", r, strlen(r));
        }
        else if (strcmp(path, "/sitemap.xml") == 0) {
            const char *sm = "<?xml version='1.0' encoding='UTF-8'?>\n"
                "<urlset xmlns='http://www.sitemaps.org/schemas/sitemap/0.9'>\n"
                "<url><loc>https://afrih.example/</loc><priority>1.0</priority></url>\n"
                "<url><loc>https://afrih.example/thesis</loc><priority>0.8</priority></url>\n"
                "<url><loc>https://afrih.example/model</loc><priority>0.8</priority></url>\n"
                "<url><loc>https://afrih.example/sectors</loc><priority>0.8</priority></url>\n"
                "<url><loc>https://afrih.example/governance</loc><priority>0.7</priority></url>\n"
                "<url><loc>https://afrih.example/impact</loc><priority>0.7</priority></url>\n"
                "<url><loc>https://afrih.example/roadmap</loc><priority>0.7</priority></url>\n"
                "<url><loc>https://afrih.example/faq</loc><priority>0.6</priority></url>\n"
                "<url><loc>https://afrih.example/invest</loc><priority>0.9</priority></url>\n"
                "<url><loc>https://afrih.example/contact</loc><priority>0.6</priority></url>\n"
                "</urlset>";
            send_response(fd, 200, "application/xml", sm, strlen(sm));
        }
        else if (strchr(path, '.'))
            serve_static(fd, path);
        else {
            send_html(fd, page_404());
        }
    }
    /* POST routes (form submissions) */
    else if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/submit-investor") == 0)
            send_json(fd, 1, "Investor inquiry received. We will respond within 5 business days.");
        else if (strcmp(path, "/submit-contact") == 0)
            send_json(fd, 1, "Message received. We will get back to you shortly.");
        else if (strcmp(path, "/submit-newsletter") == 0)
            send_json(fd, 1, "Subscribed successfully. Welcome to AFRIH.");
        else
            send_json(fd, 0, "Unknown action.");
    }
    else {
        send_html(fd, page_404());
    }
    
    close(fd);
    return NULL;
}

/* ── Server ──────────────────────────────────────────────────── */
static int start_server(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); exit(1); }
    if (listen(fd, 128) < 0) { perror("listen"); exit(1); }
    return fd;
}

int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : PORT;
    signal(SIGPIPE, SIG_IGN);
    int server_fd = start_server(port);
    printf("AFRIH C server on port %d\n", port);
    fflush(stdout);
    
    for (;;) {
        struct sockaddr_in client;
        socklen_t clen = sizeof(client);
        int cfd = accept(server_fd, (struct sockaddr *)&client, &clen);
        if (cfd < 0) continue;
        
        pthread_t tid;
        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = cfd;
        pthread_create(&tid, NULL, handle_client, fd_ptr);
        pthread_detach(tid);
    }
    return 0;
}
