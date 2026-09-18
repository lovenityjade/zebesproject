# The Zebes Project website

Public home: **https://thelovenityjade.me/zebes/**. English presentation, original
project title artwork, real in-game captures, downloads, roadmap, credits, and
four reviewed speedrun boards. Hosted on the user's existing **nexus-vps / Caddy**
installation, not on a separate site-hosting provider.

## Content and assets

- `public/` is the complete static site; no frontend framework/runtime is needed.
- `asset-provenance.json` records each exact source and hash. Existing approved
  art was reused, not regenerated. WebP conversion verifies identical RGBA pixels;
  screenshots are real captures, not generated gameplay. The title social preview
  is an existing approved capture. No new AI image assets were needed.
- Font: the existing Montserrat font, self-hosted with its OFL notice.
- Hero: existing title assets composed in CSS, subtle pointer depth/star motion.
  The visible motion switch and the OS reduced-motion preference pause animation.
- Gallery: native image dialogs, keyboard Escape, original screenshot proportions.
- All URLs work beneath `/zebes/`. No ROM, extracted standalone ROM sprites,
  saves, telemetry, tokens, third-party embeds or soundtrack recordings are served.
- This website is separate from the game build and the paused ending work.

## Leaderboard behavior

The site uses the existing receiver database on Nexus. Modes are `vanilla` and
`ngplus`; categories are `noqol` and `qol`. Ranking uses exact 60 Hz in-game frames.
The displayed fractional second is derived from frames. Real time is supplementary.
There is one best result per reviewed player identity in each category. Equal
frame counts share their rank; real time does not break ties.

`GET /zebes/api/leaderboard?mode=vanilla&category=noqol&page=1` returns up to 20
reviewed results and a total. Pending/rejected runs, Discord IDs, private payloads,
review notes, and network fingerprints are never returned. Empty and unavailable
states are distinct; there are no demo scores on the live website.

`POST /zebes/api/submissions` takes a completed native run JSON record, display
name, HTTPS YouTube/Twitch full-run video URL, and consent. A successful receipt
is **pending**, never automatic verification. The submitted video is not fetched
by the server. Admin review must validate identity, the complete video, timing,
equipment/rules and eligibility; JSON eligibility is a client claim, not anti-cheat.
The same UUID and identical details can be retried safely. Conflicting details
are rejected. Public requests are bounded and rate-limited. The game still does
not automatically upload, and Discord sign-in is not implemented.

Review commands are documented in `Server/receiver/README.md`. The reviewing
operator must reuse the same player UUID for returning runners; equal display
names alone do not establish identity. The website's PHP-free static directory
has no administration route or database access.

## Build and validation

The user requested remote testing. Run the following in the isolated copy on
**gaming-pc**, not in a running game installation:

```sh
python3 Website/build.py
python3 -m unittest discover -s Server/receiver -v
python3 Website/preview.py
# In a second terminal:
node Website/browser-check.mjs .
```

The preview binds loopback port 28742 and creates a disposable database under
`Website/validation-data/`; it never connects to production. Browser validation
checks desktop/mobile, gallery, reduced motion, menu controls, live rankings,
submission-to-review-to-ranking, HTML-safe names and exact in-game time display.
The synthetic run is deleted from the isolated database after validation.

`build.py` permits only public asset types, verifies local references/anchors,
copies `public/` to `dist/`, and writes a hash manifest outside the served tree.
Only `dist/` is deployed as the public site. Source, tests, databases, deployment
scripts, receipts, and private credentials stay outside the web root.

## Production deployment and rollback

- Static root: `/var/www/thelovenityjade.me/zebes/`.
- Receiver: `/opt/zebes-receiver`; service `zebes-receiver`; loopback `28740`.
- Database: `/var/lib/zebes-receiver/runs.sqlite`; same dedicated system user.
- Caddy exposes only the public leaderboard and submission endpoints. It strips
  Authorization, overwrites the client address header, and keeps ingestion private.
- A scoped CSP and security headers apply only to `/zebes/*`.
- Before deployment: SQLite online backup, receiver source and Caddy configuration
  backup, existing site hash checks. Staged files are hash-verified and relabeled
  for SELinux before activation. Caddy config is validated before reload.
- Backups/receipts are kept in `/var/backups/zebes-website/<deployment-id>/`.
  The deployment script prints this exact path. The SQLite changes are additive.

To roll back, restore the previous receiver files and Caddy config from that
backup, validate/reload Caddy, restart the receiver, and restore the prior `zebes`
directory if one existed. **Do not replace the live SQLite database with a backup
after real new submissions arrive.** The old receiver can run against the additive
tables without discarding new submissions. Preserve the parent website and TDNG.

## Deployment evidence — 2026-09-16

- Live HTTPS page, static assets, sitemap and all four boards return 200.
- Desktop 1440×1000 and mobile 390×844: reviewed rendered captures, no missing
  assets or JavaScript errors, no horizontal page overflow, functional lightbox,
  menu, category controls and reduced-motion support.
- Sixteen receiver tests passed on gaming-pc. The isolated browser test submitted
  a native record, confirmed it stayed private, approved it through the review
  command, and verified the correct rank and exact timer in the rendered board.
- Invalid live requests return 422; foreign-origin requests return 403.
  Private ingestion routes, Python source and SQLite files return 404 publicly.
- Cloudflare-aware rate limiting trusts CF-Connecting-IP only when Caddy's actual
  connection peer belongs to the published Cloudflare ranges. Other clients
  cannot override their network identity with that header.
- Public production database remains empty: no synthetic player or run inserted.
- Parent home, TDNG home, and the separate authentication vhost are unchanged.
- Backup: `/var/backups/zebes-website/20260916T230433Z/` on nexus-vps.
- Evidence: `Website/verification.json`; local full captures and receipts remain
  under `Build/Website/`. Public rankings read the real database, not fixtures.

Caddy uses a single path-only URI rewrite; its automatic directive ordering must
not place a full `rewrite` ahead of a separate prefix strip. References:
[URI directive](https://caddyserver.com/docs/caddyfile/directives/uri),
[Cloudflare IPv4 ranges](https://www.cloudflare.com/ips-v4/),
[Cloudflare IPv6 ranges](https://www.cloudflare.com/ips-v6/).

## Player help and seed map preview — 2026-09-17

`/zebes/help.html` contains the English player guide and FAQ, linked from the
main navigation, homepage FAQ and footer. It covers ROM setup, controls,
randomizer generation, tracker interpretation, saves, troubleshooting and
speedrun rules. Native HTML disclosures work without JavaScript; `help.js`
handles only the mobile navigation. The guide distinguishes the public alpha
from the unreleased seed atlas, travel and changed refill default.

The homepage `#seed-map` showcase uses the player's unmodified seed 15092602
capture, with a gallery enlargement and an explanation of shuffled connections.
Its provenance is recorded in `asset-provenance.json`. No generated art was used.

Static references were validated on gaming-pc. Headless Chromium checked desktop
and mobile overflow, screenshot enlargement, FAQ disclosure and mobile menu;
production HTML/assets were checked against source hashes. Only static files
were published on nexus-vps; the receiver and leaderboard database were untouched.

### French Canadian pages

`/zebes/fr/` and `/zebes/fr/help.html` are authored French translations with
English/French navigation and alternate-language metadata. No translation
service is used. `Localization/site.fr-CA.tsv` includes visible copy, image
alternatives, accessibility labels and public leaderboard messages.

`python Website/build.py` regenerates both French pages using `localize.py`
(`lxml` is a build dependency), fails on missing prose and checks links. Keep
English templates and the TSV as the sources. Do not edit generated French HTML
or `locale-fr.js`. Player names, API fields, mode IDs, run IDs and settings strings
are never translated. The server and rankings database are unchanged.

Validated in disposable Chromium on gaming-pc at desktop and mobile sizes,
including empty/populated leaderboards, preserved runner name, submission dialog
and FAQ. French production HTML matched the reviewed build byte-for-byte.
Backup before deployment: `/var/backups/zebes-website/20260918T022048Z-fr-ca`.
