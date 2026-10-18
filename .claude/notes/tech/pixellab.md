# PixelLab API: usage guide for the OpenSNES example-game assets

Written 2026-10-18 from the PixelLab docs only: no generation endpoint was called. Sources, cited inline:

- **[llms]**: `llms.txt` (the API's own summary).
- **[ep:/path]**: the endpoint's description and parameters in `endpoints.md` / `openapi.json` (spec `version: dev`, fetched for this work).
- **[spec-info]**: the spec's top-level `info.description`. **[tag:X]**: the spec's section blurb for tag X.
- **[web:/docs/...]**: pages on pixellab.ai. **[mcp-docs]**: `https://api.pixellab.ai/mcp/docs`. **[api-page]**: `https://www.pixellab.ai/pixellab-api` (USD price table). **[tos]**: `/termsofservice`. **[faq]**: `/docs/faq`.
- **UNCONFIRMED** marks anything no page states outright.

The web tool pages describe the Aseprite/Pixelorama/Creator tools. Their names differ from the endpoint names (e.g. "Create S-M image" = bitforge, "Create M-XL image" = pixflux, "Create S-XL image (Pro)" = `generate-image-v2`). Where an API endpoint and a web page disagree, the endpoint text wins for API use, and the guide notes the conflict.

---

## 1. How to call it

### 1.1 Base URL and auth
- Base URL `https://api.pixellab.ai/v2`. Every request carries `Authorization: Bearer <token>`, and the body is JSON (`Content-Type: application/json`) [llms, spec-info].
- The token comes from `https://pixellab.ai/account` [spec-info]. The MCP server takes the same Bearer token [web:/mcp].
- `GET /balance` returns `{credits: {type:"usd", usd}, subscription: {type:"generations", status, plan, generations, total}}`. `generations` is what remains in the billing period [ep:/balance, schema BalanceResponse]. Our plan string is "Tier 2: Pixel Artisan".

### 1.2 Images in requests
- Every image input is a `Base64Image` object, `{"type":"base64","base64":"<data>","format":"png"}`. Only `base64` is required, and `additionalProperties` is false [schema Base64Image].
- Some Pro endpoints wrap it with its size: `{"image": Base64Image, "width": W, "height": H}` (`ReferenceImage` / `StyleImage`, max 512 a side) [schema StyleImage; ep:/edit-images-v2 example].
- Pro Flash wraps it as `{"image": Base64Image, "size": {width, height}, "usage_description": "..."}` [schema ProFlashReferenceImage].
- Responses return base64 as a data URI (`"data:image/png;base64,..."`) [ep:/create-image-pixflux example]. Strip the prefix before decoding.

### 1.3 Uploads (`POST /uploads`)
- `POST /uploads` is a multipart form with field `file` (PNG, JPEG, "any image Pillow reads"). It is free and stores the image for 3 days [ep:/uploads].
- It returns `{ref: "upload:<uuid>", upload_id, width, height, n_frames, expires_in_days}`. `n_frames > 1` means an animation was kept intact [schema UploadedImage].
- The `ref` goes "wherever a tool takes an image URL" [schema UploadedImage; tag:Uploads].
- In the REST spec, image fields are `Base64Image` objects with no URL field. The image-URL inputs are documented on the **MCP tools** (`image_url`, `reference_image_url`, `frame_urls`, "https, data: or `upload:<id>`") [mcp-docs]. **UNCONFIRMED:** whether any REST endpoint accepts an `upload:` ref. Plan on base64 for REST.
- `POST /uploads/{ticket}` is the same upload, authorised by a 15-minute URL from the MCP tool `get_upload_url` [ep:/uploads/{ticket}].

### 1.4 Sync vs async
Read from each endpoint's 200/202 response schema in the spec.

**Synchronous**: the result is in the response body.
| Endpoint | Where the PNG is |
|---|---|
| `create-image-pixen`, `create-image-pixflux`, `create-image-bitforge`, `image-to-pixelart`, `rotate`, `inpaint`, `remove-background`, `resize`, `unzoom` | `image.base64` |
| `animate-with-text` (v1), `animate-with-skeleton` (v1), `correct-pixelart`, `reduce-colors` | `images[]` (`reduce-colors` also returns `palette`, `n_colors`) |
| `estimate-skeleton` | `keypoints` |
| `enhance-*-prompt` | `enhanced_prompt` |
| `image-to-text` | `text` |
| `talking-gif` | `gif_base64` |
| `lip-sync` | a frame plan |
| `characters/{id}/portrait` | a URL |
| `objects/{id}/select-frames`, `dismiss-review`, `uploads` | — |

**Asynchronous** (202 or a job id): everything else. That covers every Pro endpoint, every v3 endpoint, characters, objects, animations, tilesets, tiles-pro, isometric tiles, map-objects, UI, fonts, portraits, vocal-animation, VFX, normal-map, `edit-image*`, `inpaint-v3`, all Pro Flash endpoints, `image-to-pixelart-pro*` and `create-image-pixflux-background` [spec responses].

Async endpoints return one of these:
- `background_job_id`. Most endpoints.
- `background_job_id` plus a resource id: `character_id`, `object_id`, `tileset_id`, `ui_asset_id`.
- `background_job_ids[]`, one per direction: `animate-character`, `characters/animations` [ep:/animate-character "Each direction creates a separate background job"].
- A `submissions` list: `objects/{id}/animations`.

### 1.5 Polling
- `GET /background-jobs/{job_id}` returns `{id, status: processing|completed|failed, created_at, last_response, usage}` [ep:GET /background-jobs].
- While a job is queued, `last_response.queue_position` and `estimated_wait_seconds` show it is waiting for a GPU. "A waiting job is not stuck" [ep:DELETE /background-jobs].
- Recommended interval: 5–10 s, or 2–5 s on the v3 routes. Stop at `completed` or `failed`. On failure the reason is in `last_response.detail` [ep:/animate-with-text-v3 Python example].
- Where results land on completion:
  - Images: `last_response.images[]` (v3, Pro, animations) or `last_response.image.base64` (pixflux-background) [ep:/create-image-pixflux-background, ep:/animate-with-text-v3].
  - Fonts: `last_response.images[0]` (atlas) plus `last_response.ttf_base64` [ep:/generate-font-pro].
  - VFX: `last_response.files` (public download links for gif, png spritesheet + json, game-pack zip) [ep:/vfx].
- Resources also have their own GET:
  - Characters: `GET /characters/{id}`. Rotation PNG URLs are public, of the form `https://supabase.pixellab.ai/storage/v1/object/public/pixellab-characters/{user}/{char}/rotations/{dir}.png`. Exports: `GET /characters/{id}/zip` (rotations plus keypoints) and `/spritesheet` [ep:GET /characters/{id}; tag:Character Library].
  - Objects: `GET /objects/{id}` and `/spritesheet`.
  - Tilesets: `GET /tilesets/{id}`. It answers **423** while still generating and also serves side-scroller sets [ep:GET /tilesets/{id}]. Side-scroller sets also have `GET /tilesets-sidescroller/{id}`.
  - Others: `GET /tiles-pro/{id}`, `/isometric-tiles/{id}`, `/map-objects/{id}`, `/ui-assets/{id}` (`image_url`), `/portrait-character-pro/{job}` (`last_response.images[0]`), `/vocal-animation/{job}` (poll every 10–15 s), `/generate-font-pro/{job}`.
- Pro Flash results: `last_response` carries PNGs, `image_url`, a durable `source_image_id` and the actual billing. They can also be downloaded at `/mcp/images/{job}/download` [ep:/create-image-pro-flash].
- `DELETE /background-jobs/{id}` cancels a job and frees the concurrency slot. A queued job is not charged; a running one may finish and be charged [ep:DELETE /background-jobs].
- Jobs "are automatically cleaned up after completion" [ep:GET /background-jobs]. Download results promptly.
- MCP map objects are "Auto-deleted after 8 hours" [web:/mcp].

### 1.6 Errors and limits
| Code | Meaning (spec descriptions) |
|---|---|
| 401 | Invalid API token |
| 402 | Insufficient credits / generations |
| 403 | Resource belongs to another user; `vocal-animation`: "Requires a paid plan" |
| 404 | Not found or not yours |
| 409 | Direction already in the animation group (pass `replace_existing` on objects) [ep:/animate-character, ep:/objects/{id}/animations] |
| 422 | Validation (`detail[]`), e.g. bad sizes, multiples of 4, mutually exclusive fields |
| 423 | Tileset still generating |
| 429 | "Too many requests" on sync routes; "Concurrency limit reached" / "Too many concurrent jobs" on async ones |
| 529 | "Rate limit exceeded" (sync legacy routes and tilesets) |
| 503 | `vfx`, `image-to-text` |

- **Concurrency is limited per account, but no page gives the number. UNCONFIRMED.** The MCP docs mention "a rate-limited call that only landed 5/8 directions" for `animate_character` [mcp-docs]. After a multi-direction call, check that every direction's job exists, and resubmit the missing ones with the `animation_group_id`.
- Validation is done up front "so an oversized request costs nothing instead of failing on a worker" [ep:/edit-image-pixen]. Failed tileset jobs are not charged [web:/docs/tools/create-tileset].

### 1.7 Python SDK and MCP server
- **Python SDK** (`pip install pixellab`, `pixellab.Client(secret=...)`, `.from_env()`, `.from_env_file()`).
  - The GitHub README says it targets the **v1** API (pixflux, bitforge, skeleton/text animation, inpainting, rotation) [web:github pixellab-python].
  - The v2 spec's examples nevertheless call methods such as `client.create_character_v3`, `animate_with_text_v3`, `generate_tileset`, `create_tiles_pro`, `generate_font_pro` and `get_background_job` [ep:* examples].
  - **UNCONFIRMED** which PyPI version wraps which v2 endpoints. For a pipeline, calling REST directly with `requests` is the safer choice (the spec is complete).
  - A JS SDK also exists [llms].
- **MCP server**:
  - Remote endpoint `https://api.pixellab.ai/mcp`, HTTP transport, Bearer header. For Claude Code: `claude mcp add pixellab https://api.pixellab.ai/mcp -t http -H "Authorization: Bearer ..."` [web:/mcp].
  - "Requires an MCP-compatible client and an active subscription" [web:/docs/ways-to-use-pixellab].
  - It wraps far more than the GitHub README's four tools [web:github pixellab-mcp vs mcp-docs]: characters (`create_character` modes standard/pro/v3, states, animate), top-down and side-scroller tilesets, isometric tiles, objects, map placement, `create_tiles_pro` / `create_path_tiles` / `create_building_kit`, portraits/voice/fonts, the image tools, UI, `unzoom_image` / `correct_pixelart` / `reduce_colors`, `image_to_pixelart*`, the `animate_*` tools, `vfx_workbench` ("free for subscribers"), `pixelart_workbench`, `get_balance`, `get_upload_url`, and `cancel_job` / `list_jobs` / `wait_for_jobs` (240 s default timeout) [mcp-docs].
  - MCP advice: "Prefer URL inputs over base64 … MCP clients often truncate large inline data". For custom animations, "the direction must be chosen by the user, not picked by the agent"; never set `confirm_cost` without the user's approval [mcp-docs].
  - The MCP also gives a v3 character cost of "2-9 generations depending on size", and says v3 without a reference image "is humanoid only" [mcp-docs].

### 1.8 Cost model
- **Two currencies**: subscription **generations** (our 5000) and USD **credits**. The job's `usage` says which one was charged: `{type:"generations", generations: N}` or `{type:"usd", usd}` [schema Usage]. **UNCONFIRMED**: the order in which they are drawn when both exist (presumably generations first).
- **Pro** endpoints "run on a stronger, slower, more expensive model" [spec-info]. On a subscription the Pro image grid tools cost **10 / 15 / 25 generations** by size band. This applies to `generate-image-v2`, `generate-with-style-v2`, `generate-ui-v2`, `edit-images-v2`, `inpaint-v3`, `create-tiles-pro`, `create-character-pro`, objects and states [web:/docs/tools/create-sl-image-pro, consistent-style, create-ui-elements-pro; ep:/create-1-direction-object "10–25 generations"]:
  - `generate-image-v2` / with-style / ui-v2 (by max side): ≤32 → 10 (64 images); 33–42 → 15; 43–64 → 10 (16 images); 65–85 → 15; 86–128 → 10 (4 images); 129–170 → 15; 171–256 → 10 (1 image); 257–341 → 15; 342–512 → 25.
  - `create-character-pro` / `generate-8-rotations-v2`: 1–85 px → 10, 86–113 → 15, 114–168 → 25 [web:/docs/tools/create-8-rotations-pro].
  - `create-tiles-pro`: 10 or 15, and 10–25 with style tiles [web:/docs/tools/create-tiles-pro].
  - `edit-images-v2` (single image): ≤256 → 10, 257–314 → 15, 315–512 → 25 [web:/docs/tools/edit-image-pro].
- **Cheap per-call endpoints**:
  - 1 generation: `create-image-pixflux` [mcp-docs]; `edit-image-pixen` [ep]; `image-to-pixelart` [mcp-docs].
  - Pixen counts as 1 inside v3 character cost [ep:/create-character-v3].
  - `create-character-v3` = `ceil(w*h*8/65536)` from a reference, `+1` from scratch [ep] (e.g. 64×64 → 1; 128×128 → 2; 256×256 → 8).
  - `animate-with-text-v3`: 64×64 is 1 at 4/8/16 frames, 128×128 is 1/2/4, 256×256 is 4/8 [web:/docs/tools/animate-with-text-new].
  - Animation template mode: 1 per direction [ep:/animate-character].
  - Top-down tileset standard: 1–4, "usually 3 or 4 — NOT 1". Side-scroller: 2–3. Tileset pro: 10–25 [mcp-docs; web:/docs/tools/create-tileset].
  - `normal-map` 0.5. `unzoom`, `correct-pixelart`, `reduce-colors`: 0.1 [ep:/normal-map; mcp-docs].
  - `image-to-text` ≈0.09. Prompt enhancement +0.05 [ep].
  - `generate-font-pro` 15 [ep]. `vfx` lite 1, pro 6–28 [ep:/vfx]. `image-to-pixelart-pro-flash` 6 [ep].
  - Portraits: 10 at 16–64, 15 at 128/160 [mcp-docs].
  - `create-character-state`, objects: 10–25 [api-page row "Create Character State" priced as Pro].
- **Pro Flash**: "Five image generation units provisionally" per image [ep:/create-image-pro-flash]. Get the current estimate with `GET /pro-flash/cost?operation=…&width=…&height=…&n_directions=…`; the actual billing is in the finished job's `billing_stages` [ep:/pro-flash/cost, /create-character-pro-flash]. The cost and capabilities routes need auth (we got a 401 without a token). The `operation` values are **UNCONFIRMED**: read `GET /pro-flash/capabilities` first.
- **USD estimates** (credits mode), e.g. pixflux 64² $0.0079, pixen 512² $0.0169, v3 character 64² $0.041, tileset 16 px $0.0079, Pro ≤256 $0.08 / ≤341 $0.115 / ≤512 $0.15 [api-page]. "All prices are estimates and will vary because of variation in the GPU processing time" [api-page].

### 1.9 What Tier 2 ("Pixel Artisan") includes
No official page we could read lists plan contents: the pricing block renders "Loading subscription plans…" [web:/, /pricing]. What the docs do state:
- Most Pro tools need **Tier 1 or above**: tileset, texture, tiles-pro, edit-image-pro, 8-rotations-pro, consistent style. Tier 2 covers them [web:/docs/tools/*]. PixMiniMax needs tier 1+ [ep:/animate-pixminimax]. `vocal-animation` needs "a paid plan" [ep].
- **Tier-dependent canvas caps** (web tools; assumed to apply to the API, **UNCONFIRMED**):

  | Tool | Free | Tier 1 | Tier 2+ | Source |
  |---|---|---|---|---|
  | pixflux / create map | 200×200 area | 320×320 | **400×400** | [web:/docs/tools/create-image-flux, create-map] |
  | bitforge | — | 80×80 | **140×140** | [web:/docs/tools/style] |
  | image-to-pixel-art output | contradictory | contradictory | 320×320 | [web:/docs/tools/image-to-pixel-art] |
  | edit-image-pro | — | 256×256 | "larger" | [web:/docs/tools/edit-image-pro] |
  | edit-image target canvas | 200×200 | — | — | [ep:/edit-image] |

  The bitforge endpoint itself says "Maximum area 200x200" [ep]. Expect a 422 above 140×140 on Tier 2 (**UNCONFIRMED**).
- Third-party reviews say Tier 2 (Pixel Artisan, $24/month) adds a priority queue, 400×400 images and early access to experimental tools (cinevva 2026 guide; G2). **Not an official source.** Our 5000/period figure comes from `/balance` itself.

---

## 2. Endpoint chooser by asset family

Ordering follows the spec's own defaults: "Each section lists its best default first" [spec-info].
- `llms.txt` says some headings are marked `(legacy)`, but **no heading in the fetched `llms.txt` carries the mark**, and the spec has no `deprecated` / `x-` flag. The "superseded" marks below are therefore taken from the section blurbs [tag:*] and the endpoint texts.
- Legacy endpoints "still work and none are scheduled for removal" [spec-info].

### 2.1 Characters with directions
- **Standardise on `POST /create-character-v3`, reference mode** [tag:Character Creation: "recommended entry point … most-used endpoint"].
  - Give a **south-facing** `reference_image` (max 256×256). It "rotates it into 8 directional views" and stores the result under a `character_id` that animations reuse [ep].
  - `n_directions` is 4 or 8. `view` is `low top-down` / `high top-down` / `side`. `template_id` is `mannequin` for bipeds; `bear`/`cat`/`dog`/`horse`/`lion` are quadrupeds and "Must match the body type in reference_image" [ep]. Our anthropomorphic animals stand upright, so use `mannequin`.
  - `direction_images` freezes directions you drew (e.g. `east` for a side-scroller hero). With all directions supplied the character is "imported for 0.25 generations" [ep]. This is the cheap way to register hand-finished sprites in the library so the animation endpoints can use them.
  - From scratch (no reference): `image_size` 32–256. Pixen draws the south frame and "v3 without a reference image is humanoid only" [ep; mcp-docs]. `outline` / `detail` are "soft guidance — the model may not follow exactly" [ep].
  - **No `color_image` / `target_palette` on v3.** Put the reference on our palette first, and run `reduce-colors` on the rotations afterwards (§3.3).
  - Cost: 64×64 → 1 generation from a reference [ep formula].
- **The reference frame itself**:
  - `create-image-pixen` with `direction: south`, `no_background: true`, size ≥16 (square below 32).
  - Or `generate-image-v2` with a `style_image`.
  - Or `create-character-pro-flash`, which makes the south image (native 16×16 experimental, 24, 32, 32×48, 48, 64, 96×64, 96, 128) and then v3 rotations in one call [ep].
- **Alternatives**:
  - `create-character-pro` (8 directions, 10–25 gens, `image_size` 32–168, `target_palette`). Its value for us is **`style_character_id`**: an existing 8-direction character becomes the style reference "in every direction" [ep]. Use it for the cast.
  - `generate-8-rotations-v3` rotates any frame (max 256) without storing a character [ep; tag:Rotate: "Pro variant is a different model, not a newer one — try v3 first"].
- **Superseded**:
  - `create-character-with-4-directions` / `-with-8-directions` (standard template mode: 1 generation, 16–128 px, outline/shading/detail/`proportions`, `color_image` + `force_colors`) [ep]. Still the only character route whose `image_size` goes down to **16** with a forced palette. Consider it for 16×16 NPCs.
  - `rotate` (16/32/64/128 only; one direction per call; from/to view and direction) [ep; web:/docs/guides/rotating-a-character].
  - `generate-8-rotations-v2` (Pro).
- **Side-scroller characters**: only east is needed (west is a flip; "east and west are often redundant, since the generated image can be flipped" [web:/docs/options/camera]). Two routes:
  - Ask v3 for `view: side` and keep the east rotation.
  - Draw or generate the east frame and pass it in `direction_images` with a south reference.

  Our earlier observation (pixflux/bitforge ignored `view: side, direction: east`) matches the docs: those fields are "weakly guiding" on pixflux [ep], and "The view and direction controls are quite weak in this tool" [web:/docs/options/camera].

### 2.2 Character animations (walk / run / jump / idle / attack)
- **Standardise on `POST /characters/animations`** (alias semantics: `POST /animate-character`) on a library character [ep; tag:Character Creation]. The modes:

  | mode | Cost | Frames | Notes |
  |---|---|---|---|
  | `template` | 1 gen / direction, batched above ~100 px | fixed by template, "typically 4-6" | skeleton-based `template_animation_id`; `outline`/`shading`/`detail`/`text_guidance_scale` apply here only |
  | `skeleton-v3` (beta) | 2–4 gens, ~3–5 min / direction | — | "moves the character instead of redrawing it each frame — steadier identity and colours" |
  | `v3` (default without a template) | ~1 gen at 64 px | `frame_count` 4–16 even | south only if `directions` omitted |
  | `pixminimax` (beta, tier 1+) | 64×64: 1 / 1 / 2 / 6 gens at 4 / 8 / 16 / 40 frames | multiple of 4 up to 40 | — |
  | `pro` | 10–25 / direction | — | legacy |

  - Template ids seen: `angry`, `attack`, `attack-back`, `attack-left`, `attack-right`, `backflip`, `bark`, `breathing-idle`, `cross-punch`, `crouched-walking`, … The list is truncated in the spec itself; **full list UNCONFIRMED**. Read it from the error or the MCP docs.
  - `template` honours style params, and `color_image` / `force_colors` are present on the request.
  - `target_palette` works in modes pro, v3 and pixminimax. "give colors: a max_colors cap alone would pick different colours for each direction" [ep:/animate-character target_palette].
  - v3 and pixminimax accept `custom_start_frame` / `end_frame` (one direction, max 256).
  - v3 stores the reference as frame 0 unless `keep_first_frame: false` (then exactly `frame_count` frames) [ep].
  - `animation_group_id` adds directions to an existing animation; re-requesting an existing direction is a 409 [ep].
- **Stand-alone sprite (not in the library)**: `POST /animate-with-text-v3` [tag:Animate: "recommended default"].
  - Takes `first_frame`, optional `last_frame` (max 256), `action`, `frame_count` 4–16 even, `no_background`, `drift_threshold` (colour de-flicker), `enhance_prompt`. Budget: w×h×frames ≤ 524,288 [ep].
  - **No palette parameter**: run `reduce-colors` on all frames together.
  - Frame guide: 4 frames for idle loops, 8 for walk/run, 16 for attack combos [ep].
- **Exact limb control**: `estimate-skeleton`, then `animate-with-skeleton-v3` (3–15 frames, 18 joints, ≤256; 3 / 8 / 15 frames = 2 / 3 / 4 gens: "prefer one long clip") [ep]. `animate-with-skeleton` (v1, exactly 3 frames, 16/32/64/128/256) is "the older three-frame engine" [tag:Skeleton Animation].
- **In-betweens**: `interpolation-v2` (Pro, 16–128, start + end keyframe, `target_palette`) [ep].
- **Superseded**:
  - `animate-with-text-v2` (Pro; 32/64 px → 16 frames, 128+ → 4; `view`/`direction`; `target_palette`).
  - `animate-with-text` (v1, 64×64 only, always 4 frames, `negative_description`, `color_image`) [ep].
- **For 16×16 sprites**: v3 animation has no stated minimum, but its examples start at 32 [web:/docs/tools/animate-with-text-new cost table starts at 64]. **UNCONFIRMED** quality at 16 px. Generate at 32 and downscale with `resize` (≤2× per step) or by hand.

### 2.3 Enemies and NPCs
- Same pipeline as §2.1–2.2.
- For a recurring cast, `create-character-pro` with `style_character_id` set to the hero.
- Variants (damaged, armed, night, palette-swapped): `POST /create-character-state`. It applies the "same edit … consistently across all 4 or 8 directions", and the state is grouped with the source via `group_id` [ep]. Use `use_color_palette_from_reference: true` to snap the state to the source's palette, plus `target_palette`. `override_frame_size` makes room for a weapon or wings [ep].
- Quadruped enemies (dogs, cats, horses, bears, lions) use the matching `template_id` [ep].

### 2.4 Static objects and props
- **Sets of small props or icons in one style: `generate-with-style-v2`** (1–4 `style_images`). Output size = the largest style image; at ≤32 px **64 images for 10 generations** [ep; web:/docs/tools/consistent-style]. This is the most economical consistent route for 16/32 px pickups and icons. `target_palette` is supported.
- **`create-1-direction-object`** [tag:Object Creation]:
  - `size` 16–256 (square) and `view` `top-down` / `sidescroller`.
  - Up to 4 `style_images`, `item_descriptions` (one per candidate), `target_palette`.
  - The candidate count depends on size: ≤42 → 64, ≤85 → 16, ≤170 → 4, else 1. Above one candidate the object goes to `review`, and you keep some with `POST /objects/{id}/select-frames` (`indices`) or `dismiss-review` [ep].
  - 10–25 gens. Objects persist and can be animated (`objects/{id}/animations`, v3 default) or given states (`objects/{id}/states`).
- **`create-8-direction-object`** (24–168 px; `reference_image` or `style_image` or **`style_object_id`**; `target_palette`) for props that turn [ep].
- **`map-objects`**: "Cheaper and faster than a full object" [tag:Map Objects]. 32–400 px any aspect, `view` incl. `side`, outline/shading/detail, `color_image`, `init_image`. Pass `background_image` + `inpainting` (mask/oval/rectangle) to match an existing scene's style (inpainting max 192×192 area) [ep].
- **Pro Flash**: `create-object-pro-flash` (1 or 8 directions) [ep].

### 2.5 Side-scroller tileset
- **`POST /create-tileset-sidescroller`** (alias `/tilesets-sidescroller`) [ep]:
  - `lower_description` is the platform material and `transition_description` the top layer (moss, rust, snow…). `transition_size` takes 0 / 0.25 / 0.5 / 1.0.
  - `tile_size` is **16 or 32**. View is fixed "side"; the background is "Always transparent"; "No slopes: Only flat horizontal platform surfaces".
  - Other controls: outline/shading/detail, `lower_reference_image`, `transition_reference_image`, `color_image` (**no `target_palette`**), `seed`, `lower_base_tile_id` for connected sets.
  - Retrieve with `GET /tilesets/{id}`. 2–3 gens [mcp-docs].
  - **UNCONFIRMED:** the tile count of a side-scroller set (top-down is 16/25).
- Slopes, background walls and decorative fills are not covered by this endpoint. Use `create-tiles-pro` (§2.7) with `tile_view: side`, or `map-objects` with `view: side` and `background_image`.

### 2.6 Top-down tileset (Wang)
- **`POST /create-tileset`** (alias `/tilesets`) [ep; tag:Tilesets]:
  - Inputs: `lower_description` + `upper_description` (+ `transition_description`). Returns 16 tiles (25 at `transition_size` 1.0, cliffs).
  - `tile_size`: 16 or 32 in `standard` mode; 16/32/64 in `pro` mode (experimental, 10–25 gens). The web tool also lists 24×24 beta [web:/docs/tools/create-tileset], not in the API enum.
  - `view`: `low top-down` / `high top-down` only.
  - Standard mode: `shape_style` square/round, `roundness`, `enhance` (default true for shape_style; rewrites prompts and picks base colours), outline/shading/detail.
  - Pro mode: `raggedness`, `roundness`, `boundary_amplitude`/`boundary_seed`, `step_slope`, `connect_diagonals`.
  - Style and palette: `lower/upper/transition_reference_image`, `color_image`, and **`target_palette` (pro mode only)**.
  - **Chaining**: the first tileset returns base tile ids at once, and the next call can pass `lower_base_tile_id` / `upper_base_tile_id` "no waiting!" [mcp-docs]. That gives consistent grass→sand→water chains. In pro mode the reference images "make the shared terrain match an existing tileset's tile exactly" [ep:mode].
  - **Placement**: Wang corner autotiling on a vertex grid. Match on `pattern_4x4` (0 lower, 1 upper, 2 transition, 255 wildcard), not `corners` (the 25-tile cliff set has twins). `connections` is deprecated [ep]. Always read `total_tiles` [ep:GET /tilesets/{id}].
- **Alternative**: `create-tiles-pro` with `tile_feature`:
  - `tileset` gives a 16-tile corner set (square_topdown/isometric/oblique).
  - `roads` gives 18 path autotiles (square_topdown roads exactly 32 px).
  - `building` gives floor, walls, doorways, pillar and staircase; `building_layout: materials` is "more consistent".
  - Supports `target_palette` and `tile_view: top-down` (no depth). It cannot combine `tile_feature` with `style_images`.
  - "for square top-down terrain transitions, /create-tileset is the dedicated tileset model" [ep:/create-tiles-pro tile_feature].
- **Isometric**: `create-isometric-tile` (16–64, `color_image`, thin/thick/block) [ep]. Out of scope for SNES maps except as decoration.

### 2.7 Single tiles / tiles-pro
- **`POST /create-tiles-pro`**:
  - `tile_type: square_topdown` with `tile_view: top-down` (flat) or `side`. `tile_size` 16–128 ("32px recommended") [ep].
  - Number the description to get one tile per item ("1). grass tile 2). dirt tile …"); the tile count is auto-computed from the size.
  - `outline_mode: segmentation` gives "cleaner seamless edges".
  - `style_images` and `style_options` (palette/outline/detail/shading toggles) "match these tiles' style and dimensions". `target_palette`. 10–15 gens.
  - **UNCONFIRMED** that a variation tile tiles seamlessly with itself; nothing promises it.
- Tier 1+ required [web:/docs/tools/create-tiles-pro].
- The web tool "Create texture" (for tilesets) has **no matching API endpoint** [web:/docs/tools/create-texture; not in llms].

### 2.8 Parallax backgrounds (256×224 and wider layers)
- **`generate-image-v2` (Pro)**: "Start with … for the best quality" [tag:Create Image].
  - Width up to 792, height up to 688, "Maximum depends on aspect ratio (e.g. 512x512 for square, 688x384 for 16:9)". Above 170 px, one image per call.
  - `no_background` defaults to **true**: set it false for an opaque far layer, keep it true for mid and foreground layers with holes.
  - `style_image` + `style_options`, up to 4 `reference_images`, `target_palette` (then set `style_options.color_palette: false`) [ep]. 10 gens ≤256, 15 ≤341, 25 ≤512 (max side).
- **`create-image-pixen`**: "speed and sizes up to 768px".
  - Side 16–768, area ≤512×512, multiple of 4, so 512×224 and 768×336 are valid. `no_background` default false. outline/detail/view/direction, `enhance_prompt` [ep].
  - **No style image and no palette input.** Colour comes from the prompt; constrain with `reduce-colors` afterwards. About 1 generation (**UNCONFIRMED** stand-alone price; $0.0169 at 512² [api-page]).
- **`create-image-pixflux`**: area ≤400×400 on Tier 2, so 256×224 fits. `init_image` + strength (block in the layout yourself), `color_image`. 1 gen. Older and "weakly guiding" style fields [ep; web:/docs/tools/create-image-flux: "Scenery: roughly 16:9, e.g., 160x100"].
- **Horizontal wrap for scrolling layers: no endpoint promises a seamless loop. UNCONFIRMED** that any can. A workable method: shift the image by half its width, then `inpaint-v3` with a mask over the seam (32–512 px; `context_image` deprecated) [ep]. Or `inpaint-image-pro-flash`, which "preserv[es] unmasked RGBA exactly" [ep].

### 2.9 Mode 7 ground texture (up to 1024×1024, 256 tiles)
- **No endpoint produces 1024×1024.** The largest outputs are ~792×688 (`generate-image-v2`) and 768 a side (pixen) [ep].
- Mode 7 data is 256 unique 8×8 tiles anyway, so build the plane from a **tile vocabulary**:
  - `create-tileset` (top-down Wang, 16 px tiles = 2×2 SNES tiles) for terrain transitions. A 16-tile set at 16 px = 64 SNES tiles.
  - `create-tiles-pro` with `tile_view: top-down` for road, track and decoration tiles.
  - `target_palette` on both (pro mode for create-tileset).
  - Then lay out the 128×128-tile map locally and count unique 8×8 tiles.
- Alternative for a painted look: generate one 512×512 top-down image (`generate-image-v2`, `view` is not a parameter there, so put "seen from directly above" in the prompt), mirror or tile it to 1024, re-quantise with `reduce-colors`, and let our tile converter dedupe. **UNCONFIRMED** whether the result fits in 256 tiles. Probably not without heavy reuse; the tileset route is the reliable one.

### 2.10 UI panels, buttons, 9-slice
- **`generate-ui-v2` (Pro)**: individual pieces, 16 px to ~688. `color_palette` (free text, e.g. "teal and brass"), `concept_image`, `target_palette`, `no_background` default true. 10–25 gens [ep; tag:Game UI].
- **`create-ui-asset` (Pro)**: a coherent set or shape from `pieces` (rounded_rect/circle/polygon on a 512-long virtual canvas) or `elements` (button, icon_button, toolbar, tab, panel, window, health_bar, avatar…). `style_image`, `target_palette`. **Output 192–688 px only** [ep], so it is too big for SNES UI unless downscaled.
- **9-slice is not offered by any endpoint or page. UNCONFIRMED / absent.** Generate a small framed panel (e.g. 48×48 or 64×32) and cut the 3×3 regions ourselves on the 8-px grid.

### 2.11 Fonts
- **`POST /generate-font-pro`**: `description`, `weight` Bold/Regular, **`glyph_px` 8/16/32/64**. Output: an 80-glyph atlas PNG plus a TTF (A–Z, a–z, 0–9, "common game-UI punctuation"). 15 generations [ep].
- With `glyph_px: 8` it matches our 8×8 font format directly; the atlas is `images[0]`.
- **UNCONFIRMED:** the exact punctuation set, the atlas layout, the colour count of the atlas, and whether glyphs fill the full 8 px cell. Inspect the first one before committing to it.

### 2.12 Portraits for dialogue
- **`POST /portrait-character-pro` with `direction: character_to_portrait`**: turns a full-body sprite into a bust that "preserves the subject's identity/outfit". `result_size` 16/32/48/64/128/160, `target_palette`. 10 gens (15 at 128/160) [ep; mcp-docs]. This keeps portrait and sprite consistent.
- Attach it to the character (`POST /characters/{id}/portrait`, free).
- Then `POST /vocal-animation` (paid plan; `mood` neutral/happy/angry/sad/surprised; `viseme_count` 3/5/7/12, "3 for tiny portraits") generates the mouth frames once per expression [ep].
- `lip-sync` returns a free frame plan (`frame_ms`, `hold_ms`), which a SNES dialogue engine can drive itself [ep].
- Alternative: `generate-image-v2` at 48–64 px with the hero portrait as `style_image`.

### 2.13 VFX and particles
- **`POST /vfx`**:
  - `artist: lite` costs 1 generation for most effects; `pro` costs 6 (8 with a sprite). Changes via `effect_id` + `conversation` are priced lower [ep].
  - Optional `sprite` (≤512, ≤32 frames) to build the effect around it.
  - Output: GIF, spritesheet PNG + JSON (frames, durations, pivot, anchor), optional `game_pack`.
  - Tip: "Never rescale pixel art to fit: crop it or pad it" [ep].
- **No size or palette parameter is documented** (canvas comes back in `canvas`). Run `reduce-colors` on the frames and crop or pad to 16/32/64.
- The MCP's `vfx_workbench` is "free for subscribers" [ep:/vfx; mcp-docs]. Worth using from Claude Code for many effects.
- Alternatives: `animate-with-text-v3` on a seed frame (a spark, a flame), `interpolation-v2` between two keyframes ("dissolving into particles" is a listed example) [ep].

### 2.14 Sketch or non-pixel image to pixel art
- **`POST /image-to-pixelart`** (sync, 1 gen) [ep; tag:Image to Pixel Art: "Use the standard endpoint first"]:
  - Input 16–2048, output 16–512 (Tier caps may apply, §1.9). Recommended output = 1/4 of input, same aspect.
  - `init_image_strength`: "100-300 is the useful range"; above it "returns your source image rather than pixel art". `fixer: true` is faithful mode.
  - The endpoint text also says to raise strength "(0-999)" with `fixer`. The two sentences conflict; start at 200.
- `image-to-pixelart-pro` (async; auto output size; `target_palette`; `description` to steer) and `image-to-pixelart-pro-flash` (6 gens; saved as `source_image_id`) [ep].
- Rough colour-blob sketches (layouts, mock-ups): use the sketch as `init_image` on pixflux/bitforge. Strength 300–400 for "rough shapes and colors", 400–600 for variations, 600–900 for "adding details or slightly modifying a nearly finished piece" [web:/docs/options/init-image].
- Matching a reference look: `edit-images-v2` with `method: edit_with_reference` [ep].

### 2.15 Cleanup utilities
| Endpoint | What it does | Limits | Cost |
|---|---|---|---|
| `unzoom` | recovers native pixels from an upscaled image; `quantize` 0 auto / −1 keep / 2–256 | input **≥256×256**, ≤2048² area; **result is opaque** (alpha composited on white) | 0.1 |
| `correct-pixelart` | removes stray / anti-aliased pixels, tightens palette, same size; batch frames for consistency; `strength` start 0.1 | ≤1024² area; alpha preserved | 0.1 |
| `reduce-colors` | quantises several frames onto **one shared palette**; `num_colors` or `palette_image` (≤256 colours) or auto; ordered `dithering` none/2x2/4x4/8x8 | total ≤512×512 pixels across frames (e.g. 16×64², 4×128²) | 0.1 |
| `resize` | AI resize keeping pixel style; `color_image`, `init_image` | 16²–200² area, both sides; ≤2× up or ≤50% down per step | sync |
| `remove-background` | `remove_simple_background` / `remove_complex_background`, optional `text` hint | ≤400² area | sync |
| `normal-map` | lighting maps | 8–512, ≤32 frames | 0.5 |

Sources: [ep:* for each row; mcp-docs for the 0.1 costs]. Before feeding user-supplied art to any reference input, run `unzoom`: it is "the single most common cause of disappointing results" [tag:Cleanup].

---

## 3. Consistency and control

### 3.1 Style mechanisms (strongest first, by our reading of the docs)
1. **Library style references**: `style_character_id` (create-character-pro) and `style_object_id` (create-8-direction-object). The 8 directional sprites guide every direction. The output size must be ≥ the style sprite's content size, or the job "fails fast with the required size" [ep].
2. **Character states**: `create-character-state` (+ `use_color_palette_from_reference`) and `objects/{id}/states`, for variants of one design [ep].
3. **Chained tilesets**: `lower_/upper_base_tile_id`, and in pro mode the per-terrain reference images that "match … exactly" [ep; mcp-docs].
4. **Style images on Pro image endpoints**:
   - `generate-with-style-v2` (1–4 `style_images` + `style_description`; "Use multiple style images with similar aesthetics for more consistent results" [web:/docs/tools/consistent-style]).
   - `generate-image-v2` (`style_image` + `style_options` {color_palette, outline, detail, shading} toggles, plus up to 4 `reference_images` for the subject).
   - `create-tiles-pro` `style_images` (16 max in Creator) and `create-1-direction-object` `style_images`.
   - `generate-ui-v2` `concept_image`, `create-ui-asset` `style_image`.
   - Pro Flash `style_image` + `style_options` [ep].
5. **Reference-to-rotation**: `create-character-v3` and `generate-8-rotations-v3` (`description` as "an extra hint") keep a given frame's identity [ep].
6. **Outfit transfer**: `transfer-outfit-v2` (Pro, 32–256, ≤15 frames at ≤64 px). It applies a reference outfit to existing frames: "Consistent reskin of sprite animations". `additional_instructions` carries view hints, and with `target_palette` "the palette, not the reference, sets the final colours" [ep].
7. **Edit endpoints**:
   - `edit-images-v2` (Pro, 32–512; "Edit multiple images consistently"; up to 16 frames at ≤64 px).
   - `edit-animation-v2` (2–16 frames, keeps temporal coherence).
   - `edit-image-pixen` ("pose, composition and pixel style are preserved"; 1 gen; ≤256 a side, multiples of 4).
   - `edit-image-pro-flash` (`method: reference`).
   - `inpaint-v3` / `inpaint-image-pro-flash` for local fixes [ep; tag:Edit].
8. **bitforge `style_image` + `style_strength`** (0–100, "50 = balanced") on small canvases [ep].
9. **`init_image` + `init_image_strength`** (pixflux, bitforge, inpaint, map-objects, isometric, rotate, resize, skeleton v1). Strength ranges in §2.14 [web:/docs/options/init-image].
10. **Seeds**: "Reusing the same seed will, if only small changes to other options are made, provide very similar output"; 0 = random [web:/docs/options/general].
    - Pro Flash's seed is "Recorded seed; the image provider does not promise deterministic output" [ep:/create-image-pro-flash].
    - Seeds give near-reproducibility of one call, **not** consistency across different prompts.

### 3.2 View, direction, outline, shading, detail: how strongly honoured
- **pixflux**: outline, shading, detail, view, direction and isometric are all labelled **"(weakly guiding)"** in the spec [ep:/create-image-pixflux].
- **v3 and the 4/8-direction creators**: outline/shading/detail are "soft guidance — the model may not follow exactly" [ep:/create-character-v3, /create-character-with-4-directions].
- **Web docs**: camera view "Weakly sets the perspective" [web:/docs/options/guidance]. "The view and direction controls are quite weak in this tool"; better to add an init image or say it in the prompt ("woman with glasses in profile") [web:/docs/options/camera]. "Pixel art style guidance weight: Weakly controls…" [web:/docs/options/general].
- **Pro mode of create-character-with-8-directions** "Ignores outline, shading, detail, proportions, and text_guidance_scale" [ep]. **Animation template mode** is the only animation mode that reads outline/shading/detail [ep:/animate-character].
- **Where view and direction ARE structural** (they are a trained input, not a hint): the rotation models (`create-character-v3` `view`, `generate-8-rotations-v*`, `rotate` from/to), `animate-pixminimax` `direction`/`view` caption fields, `create-tiles-pro` `tile_view` / `tile_view_angle`, `create-tileset` `view`.
- Enum values:
  - Outline: `single color black outline`, `single color outline`, `selective outline`, `lineless`.
  - Shading: `flat`, `basic`, `medium`, `detailed`, `highly detailed shading`.
  - Detail: `low detail`, `medium detail`, `highly detailed`. The character endpoints list `high detail` instead.
  - View: `side`, `low top-down` (~20°), `high top-down` (~35°); `perspective` on some character endpoints.
  - Direction: 8 compass points, `south` = facing the camera [ep; web:/docs/options/camera].

### 3.3 Palette mechanisms
| Mechanism | Where | Strength |
|---|---|---|
| `target_palette {colors[2–256], name, max_colors 2–64, smart}` | Most Pro/v3-era endpoints (list below) | **Hard**: "every pixel … is one of them". `max_colors` caps per image. `smart` (beta, single images, needs `colors`) maps objects as wholes ("a face stays skin-coloured"), +15–60 s [schema TargetPalette]. Across frames or directions "give colors: a max_colors cap alone would pick different colours for each direction" [ep:/animate-character] |
| `color_image` (+ `force_colors` on the character/animation endpoints) | pixflux, bitforge, inpaint, rotate, resize, skeleton v1, animate-with-text v1, 4/8-direction characters, template animations, tilesets (both kinds), isometric tile, map-objects | "Forced color palette, image containing colors used for palette" [ep]. Web docs: "Limit colors used: **Weakly** controls the color palette" vs "Force colors: Ensures that the generated image adheres to the palette" [web:/docs/options/color]. Our test: pixflux + swatch kept 11–13 colours, i.e. near the swatch but not exact |
| `use_color_palette_from_reference` | create-character-state | snaps to the source's palette [ep] |
| `color_palette` (free text) | generate-ui-v2, create-ui-asset | prompt-level only |
| `style_options.color_palette` | generate-image-v2, tiles-pro, Pro Flash | copies colours from the style image. Set false when using `target_palette` [ep] |
| `reduce-colors` (`palette_image` or `num_colors`) | post-process | **Exact** and **shared across frames**; dithering optional. "With dithering, only the palette's colours are used" [web:/docs/tools/reduce-colors] |
| `unzoom quantize`, `correct-pixelart` | post-process | auto palette / tightening |

- `target_palette` is on: generate-image-v2, generate-with-style-v2, pro-flash image/character/object, image-to-pixelart-pro(-flash), create-character-pro, create-character-state, create-character-with-8-directions (pro mode only), characters/animations (v3, pixminimax, pro modes), animate-with-text-v2, interpolation-v2, edit-animation-v2, transfer-outfit-v2, generate-8-rotations-v2, objects (1 and 8 directions, states, animations in pro mode only), create-tiles-pro, create-tileset (pro mode only), generate-ui-v2, create-ui-asset, portrait-character-pro [ep].
- **No palette input at all**: `create-character-v3`, `animate-with-text-v3`, `generate-8-rotations-v3`, `create-image-pixen`, `edit-image-pixen`, `inpaint-v3`, `vfx`, `generate-font-pro`, `create-tileset-sidescroller` (`color_image` only) [ep].
- **SNES rule**: pass our 15 colours as `target_palette.colors` wherever it exists, with `name` (e.g. "OpenSNES neon dusk"), which "helps the look" [schema TargetPalette]. Transparency is separate (`no_background`), so 15 + transparent maps directly.
- **Always finish with `reduce-colors` + `palette_image`** on all frames of a sprite. It guarantees the exact 15 entries even after soft-palette endpoints.
- For 2bpp assets, `max_colors: 3` plus `colors` (3 entries).
- 15-bit colour (5 bits per channel): **choose palette hexes already on the 15-bit grid** (multiples of 8 per channel). Then any exact-palette mapping stays representable. Nothing in PixelLab knows about 15-bit colour.

---

## 4. Prompting

- **What the docs say**:
  - Describe the subject plainly; canvas size matters ("Scenery: roughly 16:9, e.g., 160x100. Characters: 2:3, e.g., 48x64") [web:/docs/tools/create-image-flux].
  - For inpainting or map extension, "the description should describe what's in the middle of the selected area" [web:/docs/guides/map-tiles].
  - Put the view or facing in the words ("in profile") rather than relying on the enum [web:/docs/options/camera].
  - Number multi-item prompts for tiles-pro ("1). grass tile 2). …") [ep:/create-tiles-pro].
- **Field-specific wording**:
  - `animate-with-skeleton-v3` `description`: what the character LOOKS like as a noun phrase ("Colours, clothing, held items. Not the pose, motion, style or background") [ep].
  - pixminimax: "Describe movement, not appearance" [ep:/animate-pixminimax].
  - v3 character enhancement "never mentions facing direction … and never describes a background" [ep:/enhance-character-v3-prompt]. Follow the same rule in hand-written v3 prompts.
- **Prompt enhancers** (each tuned for one target, "not interchangeable" [tag:Enhance Prompt]; $0.002 each or +0.05 gen inline):
  - `enhance-pixen-prompt` "scales detail to image_size (small images get short focused prompts)" [ep].
  - `enhance-character-v3-prompt`.
  - `enhance-animation-v3-prompt` (`engine` v3 / pixminimax / skeleton-v3; uses the frames to ground the motion).
  - The inline `enhance_prompt: true` exists on pixen, v3 character (from scratch only, 422 with a reference), v3/pixminimax animation and object v3 animation [ep].
  - Keep the returned `enhanced_prompt` text in our asset manifest. It is the prompt that actually made the pixels (create-tileset `enhance` stores it for chaining [ep]).
- **`image-to-text`** turns an image into a 1–3 sentence generation prompt (≈0.09 gen). Useful to extract a "house style" sentence from an approved asset [ep].
- **Negative prompts**:
  - `negative_description` is **"(Deprecated)"** on pixflux [ep].
  - It is still a live field on bitforge, inpaint (v1) and animate-with-text (v1) [ep]. The web docs call it something that "can sometimes be useful" [web:/docs/options/guidance].
  - Absent from every v2/v3/Pro endpoint.
  - `extra_guidance_scale` is deprecated [ep:/create-image-bitforge].
- **`text_guidance_scale`**:
  - Default 8 on most endpoints; 3.0 on inpaint v1; range 1–10 on edit-image [ep].
  - "If it is overdone, then the image can get artifacts like over-saturation" [web:/docs/options/guidance]. Relevant to our "nuanced, not garish" goal: do not raise it above 8.
  - Not used by v3 or Pro endpoints. Only template-mode animations use it.

---

## 5. Limits and gotchas for SNES work

**Sizes (minimum / maximum / shape)**
| Endpoint | Constraint |
|---|---|
| pixen | side 16–768, area ≤512², multiples of 4, **square if a side <32** |
| pixflux | area 32²–400² (Tier 2); width/height 16–400 |
| bitforge | ≤200² area (spec) / 140×140 (Tier 2 web); skeleton keypoints best at 16/32/64 |
| generate-image-v2 | ≥16, ≤~792×688 by aspect |
| generate-with-style-v2 | size = largest style image, square, 16–512 |
| create-character-v3 | ref ≤256; from-scratch 32–256; non-square padded to square |
| create-character-pro / 8-rotations-v2 | 32–168 (ref ≤168, concept ≤1024) |
| 4/8-direction characters (old) | 16–128, canvas ~40% larger |
| pro-flash character/object | 16 (experimental), 24…128 native; custom 16–256, multiples of 4 |
| create-1-direction-object | 16–256 square |
| create-8-direction-object | 24–168 square ("use the 1-direction endpoint for 16px") |
| map-objects | 32–400 each side |
| create-tileset | tiles 16 or 32 (64 pro) |
| sidescroller tileset | tiles 16 or 32 |
| create-tiles-pro | 16–128 (roads exactly 32; connectable sets mostly ≤96) |
| isometric tile | 16–64 (>24 better) |
| animate-with-text-v3 | ≤256, 4–16 even frames, w·h·f ≤ 524,288 |
| pixminimax | ≤256, frames multiple of 4 up to 40 |
| skeleton-v3 | ≤256, 3–15 frames |
| interpolation-v2 | 16–128 |
| edit-animation-v2 | 16–256; 16 frames ≤64 px, 9 ≤80, 4 above |
| transfer-outfit-v2 | 32–256 |
| edit-images-v2 | 32–512 |
| edit-image-pixen | ≤256 a side, multiples of 4, never rescale (pad) |
| inpaint-v3 | 32–512 |
| ui-asset | **192–688** |
| generate-ui-v2 | ≥16 |
| portrait | 16/32/48/64/128/160 |
| font | glyph 8/16/32/64 |
| unzoom | input **≥256** |
| resize | 16²–200² |
| reduce-colors | total ≤512² px |
| remove-background | ≤400² |

Sources: [ep:* and schema image_size for each].

**Consequences for us**
- 16×16 is below several minimums: v3 from scratch (32), create-character-pro (32), 8-direction object (24), map-objects (32), edit-images-v2 / inpaint-v3 / transfer-outfit (32).
- 16×16-capable routes: pixen (square), pro-flash (experimental), 1-direction object, old 4/8-direction characters, tiles-pro, tilesets, generate-image-v2 / with-style (64 per call at ≤32 px), rotate, skeleton v1, portraits.
- 8×8 is reachable only through fonts (`glyph_px 8`). Everything else is drawn at 16+ and cut into 8×8 tiles by our converter.
- Sprite sizes must be square 8/16/32/64 on the SNES.
  - v3 pads to a square at max(w, h) and "Animations get their own larger canvas when the motion needs room" [ep:/create-character-v3]. **Always crop or pad animation frames back to the OBJ size** and check nothing is cut.
  - Standard 4/8-direction characters render on a canvas "~40% larger" than `image_size` [schema]. A 32 px request is not a 32 px canvas.
- **Transparency**: on by default on most Pro/v3 routes, **off** by default on pixen/pixflux/bitforge/inpaint/resize/`edit-images-v2`/`edit-animation-v2`/`transfer-outfit-v2` [ep]. pixflux transparency is a "blank background over 200x200 area" [ep], so large pixflux scenes will not be cut out.
  - `unzoom` output is opaque.
  - Any alpha that is not 0/255 must be thresholded by our converter. **UNCONFIRMED** whether outputs are strictly 1-bit alpha.
- **Frame counts**:
  - v3 animations store **frame_count + 1** frames by default (reference kept) [ep]. pixminimax returns `frame_count + 1` too [ep:/animate-pixminimax].
  - Template animations' frame count is fixed by the template [ep].
- **Counts per call**:
  - Pro grid endpoints return many candidates at small sizes (64 at ≤32 px), billed as one call. Cheap exploration, but you must pick.
  - 1-direction objects in review state must be resolved with `select-frames`.
  - Sizes just above 64/128 are cheaper than just below them on pixminimax: "the model works on an upscaled canvas" [ep].
- **Library storage**: characters, objects, tilesets and UI assets persist server-side under ids. Jobs are cleaned up, uploads expire after 3 days, MCP map objects after 8 hours. Download everything into the repo's asset sources.
- **Licensing** [tos; faq]:
  - Ownership: "You retain ownership of any content you create using PixelLab" (§3.3). "You own the copyrights to your creations, permitting usage for both commercial and non-commercial purposes" (§1.3). FAQ: commercial use yes, and Steam allows it with AI disclosure.
  - **No training**: using PixelLab outputs to train models is prohibited without written permission (§1.2, §3.3). The FAQ's only request is "that you don't use the images to train new models".
    - This binds **us**. It does not bind third parties who download our MIT/zlib SDK assets; the ToS has no clause about downstream users. **UNCONFIRMED** whether PixelLab expects a notice. Recommendation: state in `ATTRIBUTION.md` that the art was made with PixelLab and is not licensed for AI training. The owner decides.
  - **Programmatic use only through the official API**: "Programmatic usage is only allowed through our official API (api.pixellab.ai)" (§2.2). No scripting the web Creator or plugin. Building our own service or reselling API access needs PixelLab's agreement (§2.2). Our pipeline must call `api.pixellab.ai/v2` (or the official MCP) only.
  - Open RAIL-M use restrictions apply (§4.1; terms not reproduced on the page). We "bear full legal responsibility" for non-infringement (§3.3). So no prompts naming existing franchises or characters.
  - PixelLab may store images for debugging/quality (§3.4). The FAQ says inputs and outputs are not stored unless stated, but the character library clearly stores them, and character rotation URLs are **public** [ep:GET /characters/{id}]. Assume anything in the library is world-readable by URL.
  - §2.1: features and pricing may change. Pin the endpoint choices in our notes and re-check the spec before each campaign.

---

## 6. Recommended workflow for a consistent universe

Budget estimates are for 64 px characters on a subscription; they vary with size.

1. **Fix the palette families first (local, no API).** Define a small set of 15-colour palettes on the 15-bit grid:
   - characters: one shared skin and fur ramp plus per-character accents;
   - three or four environment palettes (street, interior, sky, neon);
   - UI;
   - VFX.

   Save each as a hex list (for `target_palette.colors`) and as a swatch PNG (for `palette_image` / `color_image`).
2. **Make the style anchor (the house style).**
   - Generate 3–4 candidate hero frames with `generate-image-v2`: 64×64, `no_background: true`, `target_palette` = character palette, `style_options.color_palette: false`. This costs 10 gens for 16 candidates [ep; web:/docs/tools/create-sl-image-pro].
   - Pick one, then fix it by hand or with `edit-image-pixen` (1 gen) / `correct-pixelart` (0.1).
   - Extract the house-style sentence with `image-to-text` (≈0.09) and keep it in the manifest.
3. **Register the hero.**
   - `create-character-v3` with `reference_image` = the anchor, `view` (`side` for platformers, `low top-down` for RPG maps), `n_directions` 4 or 8, `template_id: mannequin`. About 1 gen at 64 px [ep].
   - Then `reduce-colors` with `palette_image` on all rotations in one call (0.1).
   - Hand-fixed directions go back in through `direction_images` (0.25 gen import).
4. **The cast.**
   - `create-character-pro` with `style_character_id` = hero, `target_palette`, one description per animal character. 10 gens at ≤85 px [ep].
   - Or, cheaper, `generate-with-style-v2` with the hero south frame as style image (10 gens → 16 candidates at 64 px), then step 3 for each chosen frame.
   - Variants with `create-character-state` (`use_color_palette_from_reference: true`).
5. **Animations.**
   - `POST /characters/animations`: `template` mode (1 gen per direction) for stock walk/run/idle/attack where a template exists. Otherwise `v3` (`frame_count` 4 idle / 8 walk-run / 16 attack, `target_palette` with `colors`). `skeleton-v3` mode where identity drifts.
   - Download, crop or pad to the OBJ size, then `reduce-colors` per animation (all frames together).
   - Platformer jump arcs: `end_frame` interpolation in v3 mode.
6. **Portraits.** `portrait-character-pro` with `character_to_portrait`, `result_size` 48 or 64, `target_palette` (10 gens). Optionally `vocal-animation` (paid plan) and the free `lip-sync` for the dialogue engine.
7. **Environment tiles.**
   - Top-down: `create-tileset` (16 px, the same outline/shading/detail across all sets, `color_image` = environment swatch; or `mode: pro` for `target_palette`). Chain sets with `lower_base_tile_id` / `upper_base_tile_id`. 3–4 gens each standard [mcp-docs].
   - Side-scroller: `create-tileset-sidescroller` (16 px, `color_image`, chain with `lower_base_tile_id`). 2–3 gens.
   - Extra decorative and fill tiles: `create-tiles-pro` with `style_images` = tiles from the first set, `target_palette`, numbered descriptions.
   - Finally `reduce-colors` per tileset onto the environment palette.
8. **Props.**
   - `generate-with-style-v2` (style images = 2–4 approved props or tiles, 64 candidates at ≤32 px for 10 gens).
   - Or `create-1-direction-object` (`style_images`, `target_palette`, then `select-frames`).
   - Or `map-objects` with `background_image` = the level background, to blend into a scene.
9. **Parallax backgrounds.**
   - `generate-image-v2` at e.g. 512×224 or 688×384 for the far layer (`no_background: false`), with `style_image` = an approved tile or background and `target_palette` = sky palette.
   - Mid and front layers with `no_background: true`.
   - Make the loop seamless with an `inpaint-v3` pass on the shifted seam.
   - Use pixen (cheap, ≤768 wide) for exploration, then `reduce-colors`.
10. **Mode 7 planes.** Build from `create-tileset` / `create-tiles-pro` (`tile_view: top-down`) tiles on the Mode 7 palette. Assemble and dedupe locally to ≤256 8×8 tiles (§2.9).
11. **UI and font.**
    - `generate-ui-v2` for each piece (`color_palette` text + `target_palette`, `concept_image` = the first approved panel for the rest); cut 9-slices locally.
    - `generate-font-pro` with `glyph_px: 8`, then quantise the atlas to the UI palette.
12. **VFX.** `/vfx` `artist: lite` (1 gen) with `sprite` = the character, or MCP `vfx_workbench` (free for subscribers). Then `reduce-colors` onto the VFX palette and crop to 16/32/64.
13. **Every asset, every time:**
    - `unzoom` on any upscaled input before using it as a reference.
    - Keep `seed`, `enhanced_prompt`, the endpoint and the parameters in a manifest beside the asset.
    - Check `usage` on each job, and `GET /balance` between batches.
    - Never run more jobs than the (undocumented) concurrency slot allows: on 429 back off, and do not cancel and resubmit queued jobs ("cancelling it to resubmit makes the wait longer" [ep:DELETE /background-jobs]).

**Rough budget per character**
| Step | Generations |
|---|---|
| Anchor (shared) | 10 |
| v3 registration | 1 |
| 4 animations × 4 directions in template mode | 16 (or ≈1 per direction in v3 at 64 px) |
| Portrait | 10 |
| Cleanup | ≈0.5 |
| **Total** | **≈25–40** |

On 5000 generations, about 40 fully animated characters still leave room for tilesets (3–4 each), Pro backgrounds (10–25 each), UI (10–25 each) and fonts (15 each). The per-call figures come from the cited docs; the totals are our arithmetic, not a PixelLab statement.

---

### Points left UNCONFIRMED (to settle with a cheap test or a support question)
- The concurrency limit per account, and the 529 rate limit.
- Whether REST image fields accept `upload:` refs.
- What `pro-flash/capabilities` returns, and the `operation` values for `pro-flash/cost`.
- The full animation template id list.
- The tile count of side-scroller sets.
- Whether the Tier 2 caps of the web tools (pixflux 400², bitforge 140²) are the API caps.
- The font atlas layout and charset.
- Whether outputs carry only 0/255 alpha.
- Seamless self-tiling of tiles-pro variations and backgrounds.
- What the current PyPI `pixellab` package wraps.
- Tier 2's official contents (the pricing page did not render).
