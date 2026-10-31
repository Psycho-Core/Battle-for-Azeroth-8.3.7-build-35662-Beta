# 09 - Town Center register

The Founder's Point Town Center and the Razorwind Shores Commons in full: every service,
every NPC, every vendor, every item they stock, its price, its currency, and its unlock
condition. I built it from the vendor pages directly rather than from a guide, because the
guides do not print item ids and I needed the ids. Vendor-level data covers all 39 neighborhood vendors (every one with an npc id);
item-level data covers **all 39 of them (1 056 items)**, 739 of which carry a retail item id,
and every one of the 39 npc ids is confirmed against a live NPC page.

One number to be careful with: the vendor index aggregates the two neighborhoods at
1 143 items, while the per-vendor counts add up to **1 155**. The difference is exactly
Hordranin's 12, which the index leaves out of its own total, so I use my own sum and say so.

## 1. Coverage, stated up front

| Group | Vendors | Rows | Status |
| --- | --- | --- | --- |
| Vendors captured in full, row count equals the catalogue count | 35 | 755 | every name, price and gate; the id count trails only where the page cap hides a row |
| Vendors captured up to the 50 row window | 4 | 301 | Ren 96 of 146, Len 98 of 117, Gronthul 57 of 83, Jehzar 50 of 54 |
| Vendors standing as two npcs | 3 | - | Ren, Len, Yen: both pages read, rows merged under one vendor name |
| **Total** | **39** | **1 056 of 1 155** | **739 carry a retail item id, 99 rows behind the window** |

The split by neighborhood is in section 4, vendor by vendor, because several vendors stand
in both neighborhoods and a single group total would count them twice.

The CSV next to this document, `data/town-center-stock.csv`, holds the 1 056 transcribed
rows in `vendor, item, price, currency, note` form. The remaining 99 catalogue items sit
behind the 50 row page cap, and section 7.4 has the exact split.

**Second pages read this round**: npc 255325 for Ren (50 rows), 255326 for Len (50), 255319
for Yen (36). What is left is 99 rows that only exist past the page cap - section 7.4.

**Captured this round**: Ren 50 + 50, Len 48 + 50, Yen 32 + 36, Gronthul 57, Jehzar 50,
Hordranin 12 - 385 rows, 385 ids, no price left blank. Roz Copperpinch remains the only
vendor whose item names are known without prices: the vendor page prints no cost for any of
her sixteen Winter Veil pieces, so those sixteen rows carry a blank price rather than a
zero.

## 2. What is physically in the hub

Founder's Point (Alliance, zone **16105**, `/way #2352`):

| Feature | What it does |
| --- | --- |
| **Bulletin Board** | Neighborhood directory: residents, plots, neighborhood identity |
| **General Contractor** | Levels up a house (XP is claimed here, never automatically) |
| **Stormwind Portal** | North side of the hub, back to the city |
| **Endeavors Area** | Active Endeavor NPCs, tasks, progress, and the coffer |
| **Vendor row** | The decor vendors in section 4 |
| **Dye station** | Dye Crafter NPCs teach Alchemy and Inscription, take dye orders |
| **Flight point** | Local taxi connection to the neighborhood's districts |
| **Public amenities** | Beer garden, stage, moonwell, and more |
| **Homestone** | Dashboard button, 15 minute cooldown, returns the player to their own plot |

Razorwind Shores (Horde, zone **15524**, `/way #2351`) mirrors this as the **Commons**:
bulletin board, General Contractor, Orgrimmar portal, Endeavors area, vendor row, dye
station, flight point, plus **hot springs**, a **stage** and a **ceremony area** instead of
the beer garden and moonwell.

Map symbols in both zones: own house, an occupied house, an available house, local
transportation, portal, bulletin board, General Contractor, flight point.

## 3. Service NPCs

| Role | Alliance | Horde | npc id |
| --- | --- | --- | --- |
| General Contractor | **Jorvan Longmoor** | **Rotha** | 255104 / 254687 |
| Steward at the capital portal | Lyssabel Dawnpetal | Tocho Cloudhide | not confirmed |
| Last Architect (armillary, weekly level-10 cache) | The Last Architect | The Last Architect | 248854 / 253596 |
| Dye Crafter (Alchemy + Inscription) | Helmi Cooper | Haleth Turnwater | 255126 / 255125 |
| Pet decor vendor | Perry Winkles | Agratha | 267795 / 267794 |
| Cobwebs (outside the hub) | Klasa, Brumewood Hollow basement | Lonomia, Saltfang Shoals under a dock | 256750 / 240465 |
| Smugglers (cross-faction decor) | Breakwater Bight: "High Tides" Ren, "Len" Splinthoof, "Yen" Malone, Unquestionably Griftah | Runetotem's Bounty South: the same | 255222/255325, 255228/255326, 255230/255319, 267870 |

## 4. Every vendor, with ids and coordinates

### 4.1 Founder's Point - Town Center row (npc ids all confirmed)

| Vendor | Items | Currency | Coordinates | npc id |
| --- | --- | --- | --- | --- |
| Faarden the Builder | 69 | Gold | 52.0 / 38.4 | 255213 |
| Xiao Dan | 65 | Gold | 52.0 / 38.3 | 255203 |
| Balen Starfinder | 54 | Gold | 52.2 / 38.0 | 255216 |
| Trevor Grenner | 43 | Gold | 53.5 / 40.9 | 255221 | 42 ids |
| Holly Twinklebright | 21 | Gold | 54.4 / 32.8 | 271604 |
| Burlee Jinglepocket | 16 | Gold | 54.3 / 32.7 | 277593 |
| Timicky | 16 | Community Coupons | 53.1 / 38.3 | 271173 |
| Argan Hammerfist | 15 | Gold | 52.2 / 37.8 | 255218 |
| Brother Dovetail | 15 | Community Coupons | 53.0 / 38.0 | 249684 |
| Fel'orei Moonstrider | 15 | Community Coupons | 52.0 / 38.4 | 257321 |
| Griftah | 15 | Community Coupons | 53.0 / 38.0 | 260485 |
| Roshai Lightstep | 15 | Community Coupons | 52.9 / 37.5 | 265551 |
| Taifa | 15 | Community Coupons | 53.1 / 38.3 | 268106 |
| Devin Slatesmith | 13 | Gold | 52.0 / 38.4 | 257332 |
| Harlowe Marl | 13 | Community Coupons | 53.0 / 38.1 | 257897 |
| Cordula Wolfgang | 11 | Tricky Treat | 54.3 / 32.7 | 273472 |
| Perry Winkles | 9 | Gold | 53.8 / 41.6 | 267795 |
| Klasa | 5 | Gold | 58.3 / 61.7 (Brumewood) | 256750 |
| Cousin Shortkaf | 1 | Community Coupons | 55.0 / 43.4 | 263258 |

That is **19 vendors, 434 items**, fully transcribed.

### 4.2 Founder's Point - Breakwater Bight (smugglers, Horde-flavoured stock)

| Vendor | Items | Currency | Coordinates | npc id |
| --- | --- | --- | --- | --- |
| "High Tides" Ren | 84 on this npc | Gold | 62.4 / 80.2 | 255222 | rows 1-50 captured, 34 past the cap |
| "Len" Splinthoof | 48 on this npc | Gold | 62.4 / 80.0 | 255228 | **complete** |
| "Yen" Malone | 32 on this npc | Gold | 62.2 / 80.3 | 255230 | **complete** |
| Unquestionably Griftah | 3 | Community Coupons | 62.4 / 79.2 | 267870 | **complete** |

One correction came out of this pass: the id I had carried for "Len" Splinthoof, 255223, is
a slime critter called Voidclot. Len is **255228** on the Alliance side and **255326** on the
Horde side, straight off the NPC page title. The vendor index counts 117 items for Len
because it adds both npc ids together; the Alliance page sells 48, all of them captured.

### 4.3 Razorwind Shores - the Commons

| Vendor | Items | Currency | Coordinates | npc id | Item data |
| --- | --- | --- | --- | --- | --- |
| "High Tides" Ren | 66 on this npc | Gold | 39.8 / 72.8 | 255325 | 50 captured, 16 past the cap |
| "Len" Splinthoof | 69 on this npc | Gold | 39.8 / 73.2 | 255326 | 50 captured, 19 past the cap |
| Gronthul | 83 | Gold | 54.1 / 59.1 | 255278 | 57 captured, 26 past the cap |
| "Yen" Malone | 36 on this npc | Gold | 40.3 / 73.0 | 255319 | **complete** |
| Jehzar Starfall | 53 | Gold | 53.6 / 58.6 | 255298 | **complete**, 50 ids |
| Shon'ja | 48 | Gold | 54.1 / 59.0 | 255297 | **complete** |
| Botanist Boh'an | 32 | Gold | 54.0 / 58.4 | 255301 | **complete**, 31 ids |
| "Den" Nightshade | 27 | Community Coupons | 39.4 / 70.2 | 257303 | **complete**, 14 ids |
| Jolli Maxiboom | 23 | Gold | 52.4 / 54.3 | 271366 | **complete**, 13 ids |
| Hesta Forlath | 18 | Coupons, gold | 54.4 / 56.0 | 252917 | **complete** |
| Pascal-K1N6 | 17 | Community Coupons | 54.1 / 56.0 | 248525 | **complete**, 16 ids |
| Roz Copperpinch | 16 | Gold | 52.4 / 54.4 | 277592 | names captured, no prices |
| Timicky | 16 | Community Coupons | 54.9 / 57.3 | 271173 | complete (shared) |
| Lefton Farrer | 15 | Gold | 53.5 / 58.5 | 255299 | **complete**, mirror of Argan |
| Merki | 13 | Gold | 54.0 / 58.8 | 257257 | **complete**, mirror of Devin |
| Aeeshna | 12 | Community Coupons | 54.4 / 56.2 | 252605 | **complete** |
| Spew Gutsout | 11 | Tricky Treat | 52.4 / 54.4 | 273470 | **complete** |
| Agratha | 9 | Gold | 53.6 / 54.4 | 267794 | **complete** |
| Lonomia | 5 | Gold | 68.3 / 75.5 | 240465 | **complete** |
| Hordranin | 12 | Community Coupons | 54.2 / 56.2 and 53.0 / 38.0 | 250820 | **complete** |
| Cousin Shortkaf | 1 | Community Coupons | 55.2 / 61.8 | 263258 | complete (shared) |

Hesta Forlath also stands in Silvermoon City at 44.1 / 62.8 (npc 252916), which is how
players buy her stock without owning a house.

## 5. Complete stock, vendor by vendor

Every captured line is in `generated/town-center-full-stock.md`: 39 vendors, 1 056 stock
lines, one table each, with the price, the currency and the retail id - or a dash where the
id is still open. That file is generated from the CSVs by
`tools/housing/build_register_appendix.py`, so it cannot drift from the data. The sections
below are the ones worth reading rather than scanning: the big architecture vendors, the
Coupon stalls, the seasonal stalls and the gated ones.

Prices are gold unless marked. A blank price means the vendor sells the piece for a
currency or condition the mirror does not print.

Where a vendor gates stock behind Endeavor milestones, the gate is written next to the item
("first", "second", "third", "fourth" = the milestone that unlocks it). `data/town-center-stock.csv`
carries only the vendor-level count in its note column, because the mirror states the count
per vendor rather than per row; doc 09 is the per-item record.

### 5.1 Faarden the Builder (69, Town Center) - local and Stormwind architecture

Arched Wooden Bench; Broken Founder's Point Fence; Carved Stone Bench; Carved Wooden Bar
Table (100); Carved Wooden Crate (25); Coal-Fired Stovetop (100); Covered Wooden Table
(100); Empty Reinforced Wooden Barrel; Empty Stormwind Market Stand (125); Farmer's Water
Trough; Founder's Point Fence; Founder's Point Fencepost; Founder's Point Framed Torch;
Founder's Point Gravestone; Founder's Point Hay Bale; Founder's Point Lamppost; Founder's
Point Navigation Sign; Founder's Point Signpost; Founder's Point Standing Torch; Founder's
Point Street Light; Founder's Point Street Sign; Gryphon Roost (150); Hay-Filled Sturdy
Feeding Trough; Large Covered Wooden Table (125); Large Founder's Point Gravestone; Large
Stonework Fountain (150); Large Sturdy Wooden Table (125); Long Founder's Point Fence; Long
Old Founder's Point Fence; Old Broken Founder's Point Fence; Old Founder's Point Fence; Old
Founder's Point Fencepost; Open Carved Wooden Crate (25); Open Sturdy Wooden Crate; Open-Air
Sturdy Tent (75); Padded Wooden Bench; Reinforced Wooden Barrel; Reinforced Wooden Chest
(50); Restful Wooden Bench (125); Runed Stone Placard; Small Founder's Point Gravestone;
Small Stonework Fountain (75); Stonework Fountain (125); Stormwind Bean Seller's Stand
(150); Stormwind Produce Seller's Stand (150); Stormwind Spice Merchant's Stand (150); Sturdy
Brazier; Sturdy Covered Wagon (125); Sturdy Feeding Trough; Sturdy Open Wagon (125); Sturdy
Roofed Wagon (125); Sturdy Sheltering Tent (125); Sturdy Weapon Rack (100); Sturdy Wine Press
(100); Sturdy Wooden Bench (75); Sturdy Wooden Chair (50); Sturdy Wooden Crate; Sturdy Wooden
Table (100); Sturdy Wooden Trellis (150); Sturdy Wooden Wheelbarrow (125); Tall Founder's
Point Gravestone; Tall Sturdy Wooden Chair (75); Weather-Treated Wooden Table (100);
Well-Built Well (100); Wooden Chamberstick (25); Wooden Gazebo (150); Wooden Planter Pot;
Wooden Planter Row; Worker's Wooden Desk (75).

### 5.2 Xiao Dan (65, Town Center) - Stormwind interiors and lighting

Carved Wooden Chair; Charming Couch (100); Charming Seat Cushion (10); Checkered Charming
Seat Cushion (10); Circular Woolen Rug (50); Closed Folk Curtains (50); Covered Wooden Desk;
Durable Folk Valance (50); Empty Wicker Basket (50); Empty Wooden Bathtub (75); Founder's
Point Front Door (75); Goldshire Window (50); Hanging Tavern Lantern; Iron-Reinforced
Cupboard (100); Iron-Reinforced Standing Mirror (100); Long Hanging Tavern Lantern; Mounted
Founder's Point Lantern; Mounted Tavern Lantern; Open Wooden Coffin (75); Plain Interior
Doorway (100); Plain Interior Narrow Wall (75); Plain Interior Wall (100); Plush Cushioned
Chair (50); Refined Wooden Bed; Secretive Bookcase Wall (125); Short Hanging Tavern Lantern;
Short Wooden Cabinet (75); Small Covered Wooden Table; Small Fruit Platter (10); Small Sturdy
Wooden Table; Small Wooden Footstool; Small Wooden Nightstand (25); Small Wooden Stool (10);
Square Woolen Rug; Stormwind Angled Platform (50); Stormwind Beam Platform (50); Stormwind
Hall Rug (75); Stormwind Interior Doorway (100); Stormwind Interior Narrow Wall (75);
Stormwind Interior Pillar (50); Stormwind Interior Wall (100); Stormwind Large Platform
(125); Stormwind Round Platform (100); Stormwind Small Platform (75); Sturdy Fireplace (100);
Sturdy Wall Rack (125); Sturdy Wooden Bathtub (125); Sturdy Wooden Bed (100); Sturdy Wooden
Bookcase (100); Sturdy Wooden Coffin (50); Sturdy Wooden Desk; Sturdy Wooden Interior Door
(50); Sturdy Wooden Interior Pillar (50); Sturdy Wooden Shelf (75); Sturdy Wooden Washbasin
(125); "Sunrise Canyon" Painting (150); Tall Sturdy Wooden Bookcase (125); Three-Candle
Wrought Iron Chandelier (50); Tied-Open Folk Curtains (50); Wicker Basket (100); Wide
Charming Couch (100); Wooden Chamberstick (25); Wooden Coffin Lid (50); Wrought Iron
Chandelier (50); Wrought Iron Floor Lamp (50).

### 5.3 Balen Starfinder (54, Town Center) - elven architecture

Bel'ameth: Angled Platform (50), Beam Platform (50), Interior Doorway (100), Interior Narrow
Wall (75), Interior Wall (100), Large Platform (125), Round Interior Pillar (50), Round
Platform (100), Small Platform (75).
Silvermoon: Angled Platform (50), Beam Platform (50), Interior Doorway (100), Interior Narrow
Wall (75), Interior Wall (100), Large Platform (125), Round Interior Pillar (50), Round
Platform (100), Small Platform (75), Bound-Left Drapes, Bound-Right Drapes, Gilded Candelabra
(75).
Furniture: Carved Elven Bookcase (100), Circular Elven Floor Rug (50), Elegant Carved Door
(75), Elegant Elven Bathtub, Elegant Elven Canopy Bed (100), Elegant Elven Chandelier (50),
Elegant Elven Desk (75), Elegant Elven Washbasin, Elegant Padded Chair (50), Elegant Padded
Chaise (100), Elegant Padded Divan (100), Elegant Padded Footstool (10), Elegant Pillow Roll
(10), Elegant Seat Cushion (10), Elegant Storage Table, Elegant Table Lamp (25), Elegant Tied
Curtain (50), Elegant Tied-Left Curtain, Elegant Wall Drape (50), Elegant Wooden Dresser
(100), Elven Standing Mirror (100), Empty Elegant Elven Bathtub, Gemmed Elven Chest (50),
Grand Elven Bookcase (100), Grand Elven Wall Curtain (50), Homestone Doormat (25), Lovely
Elven Shelf (50), Ornate Elven Stove, Ornate Stonework Fireplace (100), Ornate Weapon Rack
(75), Rectangular Elven Floor Rug (75), Small Elegant End Table (25), Small Elegant Padded
Chair (50).

### 5.4 Trevor Grenner (43, Town Center) - plants, ground cover, boulders

Arched Violet Bellflowers; Autumn Leaf Pile (150); Brumewood Hollow Large Garden Cluster
(150); Charming Laurel Tree (150); Creeping Corner Ivy (150); Creeping Lattice Ivy (150);
Duskberry Bush (150); Duskwood Shadebrush (25); Duskwood Sycamore Shrub (125); Elwynn Apple
Tree (150); Elwynn Autumn Apple Tree (150); Elwynn Cobblestone (10); Elwynn Cobblestone
Cluster (10); Elwynn Cobblestone Pair (10); Elwynn Cobblestone Round (10); Elwynn Grass Patch
(10); Elwynn Grass Spread (25); Elwynn Small Grass Patch (10); Elwynn Whitebrush (25); Flat
Boulder (50); Founder's Point Apple Tree (150); Founder's Point Autumn Apple Tree (150);
Founder's Point Blooming Grass Patch (150); Founder's Point Large Garden Cluster (150); Gift
of Gilneas (75); Gilded Oaks Large Garden Cluster (150); Gloomrose (75); Granite Cobblestone
Long Path (50); Granite Cobblestone Path (75); Granite Cobblestone Path Arc (75); Granite
Cobblestone Path Corner (75); Hilltop Boulder (50); Outer Banks Large Garden Cluster (150);
Pink Gilnean Rose (25); Reaching Violet Bellflowers; Round-Top Boulder (50); Silvermoon
Sunrise Bush (50); Small Boxwood Bush (25); Small Poppy Cluster (25); Spiritbloom Flower
(25); Stoneveil Ridge Large Garden Cluster (150); Wild Violet Bellflowers; Young Chestnut
Tree (125).

### 5.5 Devin Slatesmith (13, Town Center) - structural tiles and stairs

Counterspiral Wooden Stairs; Large Triangular Wooden Tile (100); Large Wooden Floor Tile
(100); Large Wooden Wall Tile (100); Short Round Wooden Column (25); Short Square Wooden
Column (25); Small Triangular Wooden Tile (25); Small Wooden Floor Tile (25); Small Wooden
Wall Tile (25); Spiral Wooden Stairs (75); Tall Round Wooden Column (50); Tall Square Wooden
Column (50); Wide Wooden Staircase (75).

### 5.6 Argan Hammerfist (15, Town Center) - elven furniture

Circular Elven Table (100); Elegant Almond Table (100); Elegant Carved Bench (75); Elegant
Covered Bench (75); Elegant Curved Table (100); Elegant Elven Barrel (10); Elegant Elven
Water Well; Elven Floral Window (50); Elven Wood Crate (25); Elven Woodvine Trellis (150);
Grand Elven Bench (125); Open Elegant Elven Barrel (10); Open Elven Wood Crate (25);
Rectangular Elegant Table (100); Standing Ornate Weapon Rack (100).

### 5.7 The coupon vendors - Community Coupons

**Fel'orei Moonstrider (15)** - Westfall and Moonbrook farm set: Demont Orchard Juicer (20);
Framed Moonbrook Quilt (10); Furlbrow Farm Pumpkin Trio (10); Jansen Farm Floral Basket (10);
Maxwell Stables Saddle (20); Molsen Farm Corn Row (20); Old Moonbrook Creeping Ivy (10); Old
Moonbrook Daylight Window (20); Old Moonbrook Nighttime Window (20); Saldean Autumnal Woven
Rug (10); Sentinel Hill Picnic Table (30); Sentinel Hill Rocking Chair (20); Torquewrench
Tractor (30); Westfall Farmer's Shed (30); Westfall Harvest Lamp (10).

**Brother Dovetail (15)** - Dalaran kafa and grummle set, **11 gated by Endeavor milestone**:
Ceramic Kafa Mug (5); Commander's Kafa Mug (5); Dalaran Espresso Machine (15, fourth
milestone); Dalaran Kafa Grinder (10, third); Dalaran Kafa Table (10, third); Grummle Bedroll
(10, second); Grummle Kafa Refinery (15, fourth); Grummle Sleeping Bag (10); Grummle Tent
(10, second); Kafa Creamer (5, third); Kafa Press (10, first); Legerdemain Lounge Sign Board
(10, second); Open Sack of Roasted Kafa (5, first); Pandaren Wooden Cart (10, first); Sealed
Sack of Roasted Kafa (5).

**Timicky (16)** - Kobold candle set, all coupons: Bold Kobold Kabin (30); Candle Cage (20);
Color-Curious Candle (20); Cozy Kobold Crate (10); Droopy Longwick Rope (10); Hanging Longwick
Rope (10); Highlighting High Light (20); Hot Kobold Treasure (25); Kobold Dig-Thing (10);
Kobold Sit-Thing (10); Large Threedium Warrens Candle (25); Large Warrens Candle (20);
Longwick Rope (10); Medium Warrens Candle (15); Small Warrens Candle (10); Warrens
Candlecooker (30).

**Griftah (15)** - Amani set, **11 gated**: Amani Anvil (15, second); Amani Building Peg (2);
Amani Decorative Plinth (20, second); Amani Road Marker (15, first); Amani Water Well (15,
third); Amani Wicker Crate (10, first); Forest Troll Fence (15); Forest Troll Fencepost (10);
Shrine of Akil'zon (30, fourth); Shrine of Halazzi (30, fourth); Shrine of Jan'alai (30,
fourth); Shrine of Nalorakk (30, fourth); Steamy Romance Tablet (10); Witch Doctor's Punch
Bowl (15, third); Woven Forest Troll Rug (15, first).

**Roshai Lightstep (15)** - Maruuk centaur and pet set: Bakar's Dinner (5); Bakar's Favorite
Ball (5); Bakar's Napping Rug (10); Bakar's Snack (5); Large Ornate Khanam's Rug (10);
Maruukai Barricade (10); Maruukai Chef's Stove (15); Maruukai Feast Table (10); Maruukai
Storage Basket (5); Maruukai Wooden Table (10); Open Maruukai Storage Basket (5); Ornate
Khanam's Rug (10); Pet Food and Water Tray (10); Tapered Maruukai Barricade (10); Wide
Maruukai Barricade (10).

**Taifa (15)** - Tortollan sea set: Ancient Memories of the Sea (30); Apothecary's Display
Rack (25); Beguiling Memories of the Sea (30); Collector's Display Rack (25); Compact Cookfire
(20); Empty Display Rack (20); Half-Shell Hot Pot (10); Mason's Display Rack (25); Protected
Scroll Case (5); Sea Glass Bauble (15); Sea Glass Lamp Post (15); Sealeather Sack (10);
Tortollan Tarp Tent (20); Tortollan Traveler's Chest (15); Tortollan Traveler's Tincture (5).

**Harlowe Marl (13)** - Loamm niffen set, **9 gated**: Ceramic Loamm Bowl (5); Decorated
Underground Table (15, third); Gooey Niffen Jar (5); Hearty Niffen Grub (5); Kilnmaster's
Bucket (5); Loamm Archway (20, third); Loamm Bartering Stall (20, fourth); Loamm Wheelpot
(10, second); Loammy Soil (1, second); Strong Sniffin' Incense (2, first); Underdecorated
Underground Table (15, third); Zaralek Candles (2, first); Zaralek Snail Cart (20, fourth).

**Unquestionably Griftah (3)** - Griftah's Mystical Polter-Urn (10); Griftah's Resizing
Hex-Skull (10); Griftah's Torch of Rotation (10).

**Cousin Shortkaf (1)** - Stack of Kafa Mugs (10).

### 5.8 Seasonal vendors

**Holly Twinklebright (21, gold)**: Buckled Witch Hat (250); Candlelit Smiling Jack-o'-Lantern
(500); Elaborate Heartsbane Doom Catcher (100); Enchanted Hallow's End Candle (100); Eternally
Powered Glowskull (100); Festive Red Candle; Gilded Red Wrapped Gift; Heartsbane Candle (100);
Heartsbane Doom Catcher (100); Heartsbane Ritual Markings (500); Large White Winter Veil Gift;
Ominous Heartsbane Banner (500); Red Gift Bow; Simple Hallow's End Candle (100); Small Green
Wrapped Gift; Sparsely Decorated Winter Veil Pine Tree; Spooky Scarecrow (500); Stitched
Winter Veil Stocking; Strand of Winter Veil Lights; The Ornament of Midnight; Unlit Smiling
Jack-o'-Lantern (500).

**Burlee Jinglepocket (16, gold)**: Candy Cane Rope Line; Candy Cane Rope Pole; Gilded Black
Gift Box; Gilded Black Wrapped Gift; Large Red Winter Veil Gift; Long Strand of Winter Veil
Lights; Ornate Winter Sled; Pristine Winter Veil Pine Tree; Purple Gift Bow; Small Candy Cane;
Small Green Winter Veil Gift; Small Red Winter Veil Gift; Star-Touched Winter Veil Pine Tree;
Striped Winter Veil Stocking; Winter Veil Mistletoe; Winter Veil Wreath.

**Cordula Wolfgang (11, Trickly Treat currency)** - Hallow's End: Candlelit Ogre
Jack-o'-Lantern (20); Candlelit Shrieking Jack-o'-Lantern (20); Elaborate Heartsbane Ritual
Markings (15); Filled Candy Bucket (20); Heartsbane Banner (25); Heartsbane Stone Circle (15);
Horrific Head of the Horseman (20); Looming Heartsbane Effigy (25); Ominous Heartsbane Shrine
(25); Unlit Ogre Jack-o'-Lantern (20); Unlit Shrieking Jack-o'-Lantern (20). All cost
**Tricky Treat**.

**Jolli Maxiboom (23, Horde, gold)** - the Horde mirror of Holly plus Brewfest: Brewfest Crate
(100); Traditional Brewfest Stein; and the same Hallow's End / Winter Veil catalogue as
Holly Twinklebright.

### 5.9 Pet vendors

**Perry Winkles (Alliance) and Agratha (Horde)** stock the identical nine pieces, 50g each:
Cherished Pet's Rug; Cozy Bird Nest; Cozy Lightbloom Lilypad; Crossroads Pet Cage; Crude Pet
Cage; Loyal Companion's Plinth; **Pepe** (unpriced on the mirror); Silvermoon Dragonhawk
Incubator; Westfall Pet Cage.

Pepe being on the pet vendor is the neat part: the census-era mascot is a housing decor item
sold by the neighbourhood pet NPC, and it ties straight into the Pepe quest chain in doc 04.

### 5.10 Cobwebs

**Klasa (Alliance, Brumewood Hollow basement) and Lonomia (Horde, Saltfang Shoals under a
dock)** stock the identical five cobwebs at 10g each: Ceiling Cobweb; Large Dangling Cobweb;
Pillar Cobweb; Small Dangling Cobweb; Tented Cobweb. Both are deliberately outside the hub -
a run to the graveyard or the dock is the joke.

### 5.11 Horde: Botanist Boh'an (32, gold) - desert flora and slate

Azsharan Firespear Tree (100); Barrens Hosta Bush (25); Cragthorn Highlands Large Garden
Cluster (150); Dry Razorwind Grass Patch (75); Flat Boulder (50); Flowering Durotar Cactus
(50); Granite Cobblestone Long Path (50); Granite Cobblestone Path (75); Granite Cobblestone
Path Arc (75); Granite Cobblestone Path Corner (75); Hardy Razorwind Grass Patch (75); Hilltop
Boulder (50); Nagrand Blueberry Bush (25); Pink Razorwind Paintbrush (25); Razorwind Acacia
Tree (125); Razorwind Blooms Large Garden Cluster (150); Razorwind Cactus Large Garden Cluster
(150); Razorwind Fighting Cactus (75); Razorwind Flamebrush (10); Razorwind Gobtree (75);
Razorwind Palm Tree (150); Razorwind Succulent Palm (150); Razorwind Tumbleweed (10); Red
Razorwind Paintbrush (25); Round-Top Boulder (50); Runetotem's Bounty Large Garden Cluster
(150); Saltfang Shoals Large Garden Cluster (150); Slate Cobblestone (10); Slate Cobblestone
Pair (10); Slate Cobblestone Path (10); Slate Cobblestone Trio (10); Sunset Aster Flowers (10).

### 5.12 Horde: "Den" Nightshade (27, coupons) - Consortium energy set

Complete Guide to K'areshi Wrappings, Vol. 11 (5, first milestone); Consortium Energy Banner
(15, third); Consortium Energy Barrel (10, second); Consortium Energy Collector (20, fourth);
Consortium Energy Crate (10, second); Consortium Glowpost (10, third); Demont Orchard Juicer
(20); Empty Consortium Energy Barrel (5, second); Empty Consortium Energy Crate (5, second);
Framed Moonbrook Quilt (10); Furlbrow Farm Pumpkin Trio (10); Jansen Farm Floral Basket (10);
K'areshi Holo-Crystal Projector (5); K'areshi Incense Burner (2, first); Maxwell Stables Saddle
(20); Molsen Farm Corn Row (20); Oath Scale (5); Old Moonbrook Creeping Ivy (10); Old Moonbrook
Daylight Window (20); Old Moonbrook Nighttime Window (20); Saldean Autumnal Woven Rug (10);
Sentinel Hill Picnic Table (30); Sentinel Hill Rocking Chair (20); Tazaveshi Hookah (10,
fourth); Torquewrench Tractor (30); Westfall Farmer's Shed (30); Westfall Harvest Lamp (10).

### 5.13 Horde: Hesta Forlath (18) - Silvermoon wine and paintings

Coupons: 590 Quel'Lithien Red (5); 590 Quel'Lithien Red Display Bottle (10, first); Astalor's
Hookah (5); Eversong Crystal Glass (5); Sin'dorei Artisan's Easel (5, first); Thalassian Chest
(5).
Milestone-gated coupons: Artisanal Display Tent (15, third); Sin'dorei Wine Display (15,
third); Thalassian Academy Dictation Device (10, second).
**Gold and gated by other conditions**: Silvermoon Painter's Cushion (150, requires the
achievement *Ever Painting*); and seven "Unframed Painting" pieces at **1 500g each**, all
requiring a **Deed of Patronage**: Autumnal Eversong, Brunch and a Book, Fresh Canvas,
Isolation, Reclamation, River's Protectors, The Fallen Protectors, The Light Blooms.

### 5.14 Horde: Lefton Farrer and Merki (gold, mirrors of the Alliance vendors)

**Lefton Farrer (15)** is the Horde copy of Argan Hammerfist, same fifteen elven pieces at the
same prices: Circular Elven Table (100); Elegant Almond Table (100); Elegant Carved Bench
(75); Elegant Covered Bench (75); Elegant Curved Table (100); Elegant Elven Barrel (10);
Elegant Elven Water Well; Elven Floral Window (50); Elven Wood Crate (25); Elven Woodvine
Trellis (150); Grand Elven Bench (125); Open Elegant Elven Barrel (10); Open Elven Wood Crate
(25); Rectangular Elegant Table (100); Standing Ornate Weapon Rack (100).

**Merki (13)** is the Horde copy of Devin Slatesmith: Counterspiral Wooden Stairs; Large
Triangular Wooden Tile (100); Large Wooden Floor Tile (100); Large Wooden Wall Tile (100);
Short Round Wooden Column (25); Short Square Wooden Column (25); Small Triangular Wooden Tile
(25); Small Wooden Floor Tile (25); Small Wooden Wall Tile (25); Spiral Wooden Stairs (75);
Tall Round Wooden Column (50); Tall Square Wooden Column (50); Wide Wooden Staircase (75).

That mirroring is a deliberate design choice worth copying: **the structural and elven
furniture sets are faction-neutral in stock and identical in price**, so a build looks the
same in either neighborhood. Only the local architecture, plants and seasonal stock differ.

### 5.15 Horde: Pascal-K1N6 (17, coupons) - Mechagon tinkering

11 of 17 gated by Endeavor milestone. Blue-Glo Lantern (5); Dual Mechanical Gauge (2);
Dual-Action Turbo Pro Lever (2, first); Galvanic Storage and Maintenance Device (15, fourth);
Ineffective Mechanical Privacy Screen (10, third); Jury-Rigged Electrical Couple (2, second);
Lively Pistons (2); Mad Science Blueprints (5, first); Mechagon Control Console (10, second);
Mechanical Gauge (2); Miniature Charging Station (20, fourth); Safety Electrical Cabling (5,
second); Sticky Lever (2, first); Sturdy Drive Belt (2); Triple-Tested Steam Valve (2);
Well-Oiled Machine Cog (2, first); Z-205 Mechanical Device (10, third).

This vendor is the one I care about most for the port: every piece is a 8.3.7 Mechagon prop,
so it is directly harvestable, and the gating structure is a textbook four-tier milestone
spread (2 coupons at milestone 1 up to 15-20 at milestone 4).

### 5.16 Horde: Shon'ja (48, gold) - Razorwind Shores timber and hay

The whole stall, price in gold after each piece: Crude Banded Crate (25); Durable Wooden
Chest (50); Horned Banded Barrel (10); Iron-Reinforced Crate (25); Jagged Orgrimmar Trellis
(150); Large Razorwind Farmer's Hay Pile (25); Large Razorwind Gazebo (150); Leather-Banded
Wooden Bench (125); Logger's Picnic Table (100); Long Orgrimmar Bench (100); Loose Wisps of
Hay (10); Open Crude Banded Crate (25); Open Horned Banded Barrel (10); Open Iron-Reinforced
Crate (25); Open Spiky Banded Barrel (10); Painted Wood Scrap Pile (50); Painted Wood Scraps
(25); Razorwind Banded Planter (10); Razorwind Campfire Grill (50); Razorwind Construction
Crane (150); Razorwind Cooking Grill (100); Razorwind Covered Well (100); Razorwind Farmer's
Hay Pile (50); Razorwind Fisher's Rack (125); Razorwind Fishing Net (150); Razorwind
Fountain (125); Razorwind Porch Chair (75); Razorwind River Paddle (50); Razorwind Roofer's
Shingle (10); Razorwind Roofer's Shingle Pile (50); Razorwind Shores Canoe (125); Razorwind
Wheelbarrow (50); Rugged Brazier (50); Rugged Stool (10); Scattered Wisps of Hay (10); Short
Orgrimmar Bench (75); Small Jagged Orgrimmar Trellis (100); Small Razorwind Farmer's Hay Pile
(75); Sparse Razorwind Fisher's Rack (125); Spiky Banded Barrel (10); Tiny Clump of Hay (10);
Trampled Wisps of Hay (10); Tusked Gazebo (150); Tusked Weapon Stand (100); Twisted Rope Coil
(25); Wide Hide-Covered Bench (100); Wind Rider Roost (150); Windblown Wisps of Hay (10).

Every one of these has an item id in `data/town-center-items.csv` (244532 - 268028). This is
the first vendor I have captured end to end, ids included, and it is also the cheapest
furniture in either neighborhood: 44 of the 48 pieces are 150g or less, and the hay wisps
and shingles start at 10g.

### 5.17 Horde: Aeeshna (12, coupons) - the Endeavor trader

Ten of twelve gated by Endeavor milestone. Complete Guide to K'areshi Wrappings, Vol. 11
(5, first); Consortium Energy Banner (15, third); Consortium Energy Barrel (10, second);
Consortium Energy Collector (20, fourth); Consortium Energy Crate (10, second); Consortium
Glowpost (10, third); Empty Consortium Energy Barrel (5, second); Empty Consortium Energy
Crate (5, second); K'areshi Holo-Crystal Projector (5); K'areshi Incense Burner (2, first);
Oath Scale (5); Tazaveshi Hookah (10, fourth).

Aeeshna is the *Endeavor Trader* - wowhead labels the npc that way, and she stands in both
neighborhoods rather than one. Her stock is the Consortium energy set, which "Den" Nightshade
also carries at the same coupon prices. What is unique to her is the K'areshi set: the guide
volume, the holo-crystal projector, the incense burner and the Oath Scale.

### 5.18 Horde: Spew Gutsout (11, Tricky Treat) - Hallow's End

Every price is in Tricky Treats, item **33226**, not gold and not coupons. Candlelit Ogre
Jack-o'-Lantern (20); Candlelit Shrieking Jack-o'-Lantern (20); Elaborate Heartsbane Ritual
Markings (15); Filled Candy Bucket (20); Heartsbane Banner (25); Heartsbane Stone Circle
(15); Horrific Head of the Horseman (20, plus "use the Group Finder and defeat the Headless
Horseman"); Looming Heartsbane Effigy (25); Ominous Heartsbane Shrine (25); Unlit Ogre
Jack-o'-Lantern (20); Unlit Shrieking Jack-o'-Lantern (20).

He also sells two recipes that are not decor and therefore not in the decor register:
**284212 Technique: Apple Bobbing Bucket** (Inscription) and **284213 Pattern: Spidery Witch
Hat** (Tailoring), 20 Tricky Treats each.

### 5.19 Horde: Roz Copperpinch (16, gold) - Winter Veil

Candy Cane Rope Line; Candy Cane Rope Pole; Gilded Black Gift Box; Gilded Black Wrapped Gift;
Large Red Winter Veil Gift; Long Strand of Winter Veil Lights; Ornate Winter Sled; Pristine
Winter Veil Pine Tree; Purple Gift Bow; Small Candy Cane; Small Green Winter Veil Gift; Small
Red Winter Veil Gift; Star-Touched Winter Veil Pine Tree; Striped Winter Veil Stocking; Winter
Veil Mistletoe; Winter Veil Wreath.

No prices: her vendor page prints none for any of the sixteen. They are in
`data/town-center-stock.csv` with a blank price and the note "price not published", which is
the honest state rather than a row of zeros. She and Spew Gutsout share the same published
coordinates (52.4 / 54.4), which is what a holiday stall that swaps with the calendar looks
like on a map pin.

## 6. Three things this data proves that the guides do not say

1. **Endeavor milestones gate vendor stock, permanently.** Roughly a third of the coupon
   vendors' stock carries "Reach the N milestone of the endeavor". A neighborhood that never
   finishes Endeavors cannot buy those items at any price. This is a much stronger design
   lever than "Endeavors give currency", and it goes into doc 04 as the milestone model.
2. **A Deed of Patronage is a second currency for prestige decor.** Eight paintings cost
   1 500g each *and* need a Deed. Combined with the achievement-gated cushion, retail has
   three gate types on one vendor: currency, milestone, and entitlement.
3. **Prices cluster on a deliberate ladder**: 10g (ground cover, cobwebs, candles), 25g
   (small props, bushes, columns), 50g (cushions, rugs, small tables, pet beds), 75-100g
   (furniture, tiles, walls, doors), 125g (large tables, big platforms, wagons), 150g
   (trees, garden clusters, gazebos, market stands), 250-500g (holiday showpieces), 1 500g
   (paintings). That is the price table `psycho_house_decor` should copy.

## 7. Loading this into the module

Three steps, in this order. I do not hand-edit the CSVs any more; every row comes from a
capture file, so a rerun replaces a vendor instead of double counting it.

```bash
# 1. one vendor at a time, from a capture in data/captures/
python3 tools/housing/merge_capture.py \
    --vendor "Gronthul" \
    --file data/captures/gronthul.tsv \
    --source "wowhead npc 255278"

# 2. turn the CSVs into the two drafts
python3 tools/housing/build_drafts.py

# 3. or load the CSV straight into a world database
mysql world -e "LOAD DATA LOCAL INFILE 'data/town-center-stock.csv' \
  INTO TABLE psycho_house_vendor_stock \
  FIELDS TERMINATED BY ',' OPTIONALLY ENCLOSED BY '\"' \
  LINES TERMINATED BY '\n' IGNORE 1 LINES \
  (vendor, item, price, currency, note);"
```

`--append` is the flag for a vendor that stands as two npcs: without it the merge replaces
that vendor's rows, with it the second page adds to the first.

Then the mapping pass: for each retail item I pick the 8.3.7 game object that stands in for
it. Fences, crates, barrels, chairs, tables, lanterns, cobwebs, trees, boulders, tiles and
staircases all have direct 8.3.7 equivalents. Stormwind and Bel'ameth *architecture* does
not, which is exactly why the shell system in doc 06 exists.

### 7.1 Item ids

Names and prices are one thing; the **item id** is what actually goes into a vendor table.
I am collecting them separately, because wowhead's NPC "Sells" list paginates at 50 rows and
the pager is JavaScript, so one page fetch never returns a vendor's full list.

`data/town-center-items.csv` currently holds **739 resolved item ids**, every one read off a
wowhead sell list, none guessed:

| Vendor | Items with ids | Item id range |
| --- | --- | --- |
| "Len" Splinthoof | 98 of 117 | 235523 - 268028 |
| "High Tides" Ren | 96 of 146 | 235633 - 250913 |
| "Yen" Malone | 68 of 68 | 245298 - 266444 |
| Gronthul | 57 of 83 | 236653 - 267088 |
| Faarden the Builder | 50 of 69 | 235523 - 252004 |
| Jehzar Starfall | 50 of 54 | 235994 - 264353 |
| Xiao Dan | 50 of 65 | 235633 - 252037 |
| Shon'ja | 48 of 48 | 244532 - 268028 |
| Trevor Grenner | 42 of 43 | 245298 - 266444 |
| Botanist Boh'an | 31 of 32 | 248337 - 266444 |
| Balen Starfinder | 30 of 54 | 235994 - 246258 |
| Pascal-K1N6 | 16 of 17 | 254400 - 254416 |
| "Den" Nightshade | 14 of 27 | 280625 - 282347 |
| Brother Dovetail | 14 of 15 | 246686 - 252041 |
| Griftah | 14 of 15 | 255649 - 274539 |
| Argan Hammerfist | 13 of 15 | 241623 - 267202 |
| Jolli Maxiboom | 13 of 23 | 248101 - 282100 |
| Aeeshna | 12 of 12 | 262664 - 263048 |
| Hordranin | 12 of 12 | 250627 - 250704 |
| Spew Gutsout | 11 of 11 | 282192 - 282289 |

739 rows, 439 distinct ids: the difference is the large shared sets - the Orgrimmar
furniture runs sold by Ren, Len and Gronthul, the Stormwind runs, the holiday pieces, and
the Chamberstick that several vendors stock. Every row carries a `source` column naming the NPC page it came
from, so an id can be retraced in one step.

Sample of what is now pinned down, id first:

| Item id | Item | Vendor | Price |
| --- | --- | --- | --- |
| 235523 | Sturdy Wooden Chair | Faarden | 50g |
| 235633 | Circular Woolen Rug | Xiao Dan | 50g |
| 235994 | Ornate Stonework Fireplace | Balen | 100g |
| 242951 | Sturdy Wooden Bench | Faarden | 75g |
| 243334 | Reinforced Wooden Chest | Faarden | 50g |
| 245352 | Sturdy Wooden Coffin | Xiao Dan | 50g |
| 245575 | Bel'ameth Interior Wall | Balen | 100g |
| 245583 | Silvermoon Interior Wall | Balen | 100g |
| 246249 | Silvermoon Beam Platform | Balen | 50g |
| 246875 | Founder's Point Street Light | Faarden | 100g |
| 246876 | Founder's Point Lamppost | Faarden | 100g |
| 250249 | Founder's Point Gravestone | Faarden | 25g |
| 252037 | Covered Wooden Desk | Xiao Dan | 75g |

One item, **246106 Wooden Chamberstick**, is stocked by both Xiao Dan and Faarden at 10g -
the first confirmed case of exact shared stock between two Town Center vendors, which matters
because it means a vendor table cannot assume one item belongs to one vendor.

`data/vendor_npc_ids.csv` now carries **39 of 39** vendor npc ids and slugs (Hordranin was
the gap), so
`tools/housing/fetch_item_ids.py` can work on any vendor unattended. It tries the NPC page,
the ?sells=N variants, the item-filter endpoint with paging, and finally a per-name search,
and records which strategy produced each id so a wrong answer is traceable.

The four ids that were missing all came out of the vendor pages themselves, which link to
the wowhead npc at the top of each entry: Shon'ja **255297**, Roz Copperpinch **277592**,
Aeeshna **252605**, Spew Gutsout **273470**. Two of them are labelled on wowhead rather
than in a guide - Aeeshna is an *Endeavor Trader*, Spew Gutsout a *Hallow's End Decor
Specialist* - which is also why their stock is paid in Community Coupons and Tricky Treats
instead of gold.

### 7.2 Why 317 stock rows still have no item id

Four walls, all tested rather than assumed:

1. **50 rows per list, no paging.** `?page=2`, `&p=2`, `&pagesize=100` and `?sells=2` all
   return page one, on `/items`, on `/items/housing/decor`, with and without a vendor
   filter. The pager in the page markup is JavaScript and the query string is ignored
   server side. A vendor with 146 items (Ren) therefore shows 50 and there is no URL that
   shows the other 96.
2. **The "Sold by NPC" filter is incomplete.** It returns part of a vendor's stock: Den
   Nightshade 14 of 27, Argan Hammerfist 13 of 15. It is still the best route available,
   because what it returns is exact.
3. **Some NPCs are not indexed at all.** Burlee Jinglepocket (277593) and Roz Copperpinch
   (277592) are 12.1.7 additions: `filter=129;0;277593` answers `no items matched the criteria` on both the live and
   the PTR listing, which is also why Roz has no
   prices anywhere on the site.
4. **A filter can answer with a different vendor.** `filter=129;0;257321` (Fel'orei
   Moonstrider) came back with Den Nightshade's Westfall set, and two parallel requests
   for Jolli and Holly returned the same page. Every list is therefore validated by name
   against the register before an id is accepted: the 157 ids added this round matched
   their vendor's stock names 100 percent, and two lists that did not (Fel'orei, Lefton)
   were thrown away rather than merged.

What is left is **317 of the 917 transcribed rows**. Three routes remain, in the order I
would use them:

1. `tools/housing/fetch_item_ids.py --search-fallback` - one search request per item name.
   It works, it is slow, and it records `source` per row so a mixed result stays auditable.
2. The vendor's own wowhead NPC page, which lists up to 50 items with ids regardless of the
   decor filter. That is what closed Trevor, Boh'an, Pascal, Dovetail and Griftah.
3. The second-faction npcs section 7.3 names: same routine, different npc id.

### 7.3 The six captures that closed the vendor list

Each of the six went the same way: read the NPC page, take the sells table, diff the names
against the register, write the rows through `tools/housing/merge_capture.py`. The raw
captures are kept in `data/captures/`, one tab separated file per vendor, so a rerun is a
merge and not a retype.

| Vendor | npc id | Captured | Note |
| --- | --- | --- | --- |
| "High Tides" Ren | 255222 | 50 of 84 | page cap; the Horde copy 255325 holds the other 62 |
| "Len" Splinthoof | 255228 | 48 of 48 | complete; the Horde copy 255326 holds 69 more |
| Gronthul | 255278 | 50 of 83 | page cap, single npc |
| "Yen" Malone | 255230 | 32 of 32 | complete; the Horde copy 255319 holds 36 more |
| Jehzar Starfall | 255298 | 50 of 53 | page cap, single npc |
| Hordranin | 250820 | 12 of 12 | complete, all Community Coupons, 8 of 12 gated on an Endeavor milestone |

Two things came out of it that were not in the register before:

1. **Len's npc id was wrong**, and not subtly: 255223 is Voidclot, a slime critter in
   Voidstorm. Len is 255228 / 255326. The correction is now in
   `data/vendor_npc_ids.csv`, so 39 of 39 ids are confirmed against an NPC page.
2. **Hordranin is an Endeavor Trader, not a gold vendor.** Twelve pieces, 5 to 20
   Community Coupons each, and the note column carries the milestone each one needs. That
   is the only vendor in the register whose entire stock is gated.

The second faction pages are the remaining work on these six: 62 + 69 + 36 rows behind
npc 255325, 255326 and 255319, plus the 34 and 33 rows past the page cap on Ren and
Gronthul. Every one of them is a page read with the same routine.

### 7.4 What the 99 missing rows actually are

All 39 vendors are captured; what is left is 99 rows that only exist on the wrong side of
the 50 row window, and every one of them is counted here:

| Vendor | npc | Sells | Captured | Missing |
| --- | --- | --- | --- | --- |
| "High Tides" Ren | 255222 | 84 | 50 | 34 |
| "High Tides" Ren | 255325 | 66 | 50 | 16 |
| "Len" Splinthoof | 255326 | 69 | 50 | 19 |
| Gronthul | 255278 | 83 | 57 | 26 |
| Jehzar Starfall | 255298 | 53 | 50 | 3 on the page, 4 against the index count of 54 |

Ren's two pages share four items (the three plain interior panels and the Chamberstick), so
the 146 the index counts for Ren comes out of 84 + 66 minus those four.

One thing did come out of pushing at the cap: the NPC page and
`www.wowhead.com/items?filter=129;0;<npc>` return the **same total with a different
ordering**, so their first fifty rows are not the same fifty. Gronthul's second window held
seven items the NPC page did not (the hanging Razorwind ropes, the tusked sconce and
chandelier, the hide-covered bench and wall shelf, the iron candlelight lantern). That
trick bought 7 of the 26 rows still missing on Gronthul and would buy a few on each of the
other three if the rest of each window could be read; the window is still fifty rows, so
the union is the ceiling.

### 7.5 Price corrections this exercise forced

Syncing wowhead against the mirror I first transcribed from changed **30 prices**: 7 were
wrong and 23 were blanks that wowhead fills. The wrong ones:

| Item | Mirror said | Wowhead says |
| --- | --- | --- |
| Wicker Basket | 100g | **10g** |
| Empty Wicker Basket | 50g | **10g** |
| Wooden Chamberstick | 25g | **10g** |
| Wooden Coffin Lid | 50g | **25g** |
| Sturdy Wall Rack | 125g | **75g** |
| Sturdy Wooden Shelf | 75g | **50g** |
| Sturdy Wooden Interior Pillar | 50g | **50g** (confirmed, was on a different row) |

Rule from here on: **wowhead is the price authority, the mirror is only a stub to find the
vendor.** Every price in `data/town-center-stock.csv` that wowhead has confirmed is now
wowhead's number.

### 7.6 The 87 rows with no published price

Blank is not free. Every one of these rows is a real item with a real vendor; the page just
does not print a cost, usually because the price depends on a condition the page does not
render. I left the price column empty and listed them all here rather than guessing a
number.

| Vendor | Rows with no published price | Items |
| --- | --- | --- |
| Faarden the Builder | 17 | Broken Founder's Point Fence; Empty Reinforced Wooden Barrel; Founder's Point Fence; Founder's Point Fencepost; Founder's Point Framed Torch; Founder's Point Hay Bale; Founder's Point Standing Torch; Long Founder's Point Fence; Long Old Founder's Point Fence; Old Broken Founder's Point Fence; Old Founder's Point Fence; Old Founder's Point Fencepost; Open Sturdy Wooden Crate; Padded Wooden Bench; Reinforced Wooden Barrel; Sturdy Wooden Crate; Wooden Planter Row |
| Burlee Jinglepocket | 16 | Candy Cane Rope Line; Candy Cane Rope Pole; Gilded Black Gift Box; Gilded Black Wrapped Gift; Large Red Winter Veil Gift; Long Strand of Winter Veil Lights; Ornate Winter Sled; Pristine Winter Veil Pine Tree; Purple Gift Bow; Small Candy Cane; Small Green Winter Veil Gift; Small Red Winter Veil Gift; Star-Touched Winter Veil Pine Tree; Striped Winter Veil Stocking; Winter Veil Mistletoe; Winter Veil Wreath |
| Roz Copperpinch | 16 | Candy Cane Rope Line; Candy Cane Rope Pole; Gilded Black Gift Box; Gilded Black Wrapped Gift; Large Red Winter Veil Gift; Long Strand of Winter Veil Lights; Ornate Winter Sled; Pristine Winter Veil Pine Tree; Purple Gift Bow; Small Candy Cane; Small Green Winter Veil Gift; Small Red Winter Veil Gift; Star-Touched Winter Veil Pine Tree; Striped Winter Veil Stocking; Winter Veil Mistletoe; Winter Veil Wreath |
| Holly Twinklebright | 9 | Festive Red Candle; Gilded Red Wrapped Gift; Large White Winter Veil Gift; Red Gift Bow; Small Green Wrapped Gift; Sparsely Decorated Winter Veil Pine Tree; Stitched Winter Veil Stocking; Strand of Winter Veil Lights; The Ornament of Midnight |
| Jolli Maxiboom | 9 | Festive Red Candle; Gilded Red Wrapped Gift; Large White Winter Veil Gift; Red Gift Bow; Small Green Wrapped Gift; Sparsely Decorated Winter Veil Pine Tree; Stitched Winter Veil Stocking; Strand of Winter Veil Lights; The Ornament of Midnight |
| Xiao Dan | 9 | Carved Wooden Chair; Hanging Tavern Lantern; Long Hanging Tavern Lantern; Mounted Founder's Point Lantern; Mounted Tavern Lantern; Refined Wooden Bed; Short Hanging Tavern Lantern; Square Woolen Rug; Sturdy Wooden Desk |
| Balen Starfinder | 6 | Elegant Elven Bathtub; Elegant Elven Washbasin; Elegant Storage Table; Elegant Tied-Left Curtain; Empty Elegant Elven Bathtub; Ornate Elven Stove |
| Agratha | 1 | Pepe |
| Devin Slatesmith | 1 | Counterspiral Wooden Stairs |
| Lefton Farrer | 1 | Elegant Elven Water Well |
| Merki | 1 | Counterspiral Wooden Stairs |
| Perry Winkles | 1 | Pepe |

## 8. Acceptance checks

| Check | Rule | Result |
| --- | --- | --- |
| Roster | Every vendor stocking decor inside either neighborhood appears once | 39/39 |
| Counts | Transcribed item count equals published count | 35 of 39 exact; Ren, Len, Gronthul and Jehzar are short by the page cap only |
| Rows | Transcribed rows against the 1 155 catalogue | 1 056, the 99 remainder named in section 7.4 |
| Item ids | Every id read off a vendor page, none inferred | 739 ids on 1 056 rows |
| Currency | Gold, Community Coupons, Tricky Treat, Deed of Patronage kept distinct | verified |
| Vendor ids | Printed only when confirmed against an NPC page | 39 of 39 confirmed. Throska, a service NPC outside the vendor list, still prints no id |
| Coordinates | Every coordinate carries its `/way` map id | verified, #2351 vs #2352 |
| Prices | Printed only when the source prints them | 30 corrected against wowhead; 87 blanks remain, every one listed in section 7.6 |
| Item ids | Id traced to the item page, none inferred | 739 ids resolved, 0 guessed; 317 rows still without one, counted in section 7.2 |
| Decor to game object | Every mapped entry checked against the source index | 618 names mapped onto 187 game objects, 571 decor rows shipped, 0 invented entries |
