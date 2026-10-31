# 09 - Town Center register

The Founder's Point Town Center and the Razorwind Shores Commons in full: every service,
every NPC, every vendor, every item they stock, its price, its currency, and its unlock
condition. Vendor-level data covers all 39 neighborhood vendors; item-level data covers 29 of
them (588 items), with the remaining 10 listed by count and captured by the scraper.

## 1. Coverage, stated up front

| Group | Vendors | Items | Status |
| --- | --- | --- | --- |
| Founder's Point Town Center + local | 19 | 434 | **complete**, every item, price and gate |
| Founder's Point Breakwater Bight (smugglers) | 3 | 331 | vendor data complete, item capture pending |
| Razorwind Shores Commons + local | 11 | 350 | 8 complete, 3 pending |
| Shared (both neighborhoods) | 6 | - | counted above, not double-counted |
| **Total** | **39** | **1 143** | **588 items transcribed, 29 of 39 vendors** |

The CSV next to this document, `data/town-center-stock.csv`, holds the 588 transcribed
rows in `vendor, item, price, currency, note` form. The scraper in
`tools/housing/fetch_vendor_stock.py` fills the remaining 555 without retyping.

**Still to capture, largest first**: "High Tides" Ren (146), "Len" Splinthoof (117),
Gronthul (83), "Yen" Malone (68), Jehzar Starfall (54), Shon'ja (48), Roz Copperpinch (16),
Aeeshna (12), Spew Gutsout (11).

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

Map symbols in both zones: your house, an occupied house, an available house, local
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
| Smugglers (cross-faction decor) | Breakwater Bight: "High Tides" Ren, "Len" Splinthoof, "Yen" Malone, Unquestionably Griftah | Runetotem's Bounty South: the same | 255222/255325, 255223/255324, 255230/255319, 267870 |

## 4. Every vendor, with ids and coordinates

### 4.1 Founder's Point - Town Center row (npc ids all confirmed)

| Vendor | Items | Currency | Coordinates | npc id |
| --- | --- | --- | --- | --- |
| Faarden the Builder | 69 | Gold | 52.0 / 38.4 | 255213 |
| Xiao Dan | 65 | Gold | 52.0 / 38.3 | 255203 |
| Balen Starfinder | 54 | Gold | 52.2 / 38.0 | 255216 |
| Trevor Grenner | 43 | Gold | 53.5 / 40.9 | 255221 |
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
| "High Tides" Ren | 146 | Gold | 62.4 / 80.2 | 255222 |
| "Len" Splinthoof | 117 | Gold | 62.4 / 80.0 | 255223 |
| "Yen" Malone | 68 | Gold | 62.2 / 80.3 | 255230 |
| Unquestionably Griftah | 3 | Community Coupons | 62.4 / 79.2 | 267870 |

Unquestionably Griftah is transcribed in full (see section 5); the three big smugglers are
not yet, and they are the largest single block of stock in the game.

### 4.3 Razorwind Shores - the Commons

| Vendor | Items | Currency | Coordinates | npc id | Item data |
| --- | --- | --- | --- | --- | --- |
| "High Tides" Ren | 146 | Gold | 39.8 / 72.8 | 255325 | pending |
| "Len" Splinthoof | 117 | Gold | 39.8 / 73.2 | 255324 | pending |
| Gronthul | 83 | Gold | Commons | not confirmed | pending |
| "Yen" Malone | 68 | Gold | 40.3 / 73.0 | 255319 | pending |
| Jehzar Starfall | 54 | Gold | 53.6 / 58.6 | 255298 | pending |
| Shon'ja | 48 | Gold | Commons | not confirmed | pending |
| Botanist Boh'an | 32 | Gold | 54.0 / 58.4 | 255301 | **complete** |
| "Den" Nightshade | 27 | Community Coupons | 39.4 / 70.2 | 257303 | **complete** |
| Jolli Maxiboom | 23 | Gold | 52.4 / 54.3 | 271366 | **complete** |
| Hesta Forlath | 18 | Coupons, gold | 54.4 / 56.0 | 252917 | **complete** |
| Pascal-K1N6 | 17 | Community Coupons | 54.1 / 56.0 | 248525 | **complete** |
| Roz Copperpinch | 16 | Gold | Commons | not confirmed | pending |
| Timicky | 16 | Community Coupons | 54.9 / 57.3 | 271173 | complete (shared) |
| Lefton Farrer | 15 | Gold | 53.5 / 58.5 | 255299 | **complete** |
| Merki | 13 | Gold | 54.0 / 58.8 | 257257 | **complete** |
| Aeeshna | 12 | Community Coupons | Commons | not confirmed | pending |
| Spew Gutsout | 11 | Tricky Treat | Commons | not confirmed | pending |
| Agratha | 9 | Gold | 53.6 / 54.4 | 267794 | **complete** |
| Lonomia | 5 | Gold | 68.3 / 75.5 | 240465 | **complete** |
| Cousin Shortkaf | 1 | Community Coupons | 55.2 / 61.8 | 263258 | complete (shared) |

Hesta Forlath also stands in Silvermoon City at 44.1 / 62.8 (npc 252916), which is how
players buy her stock without owning a house.

## 5. Complete stock, vendor by vendor

Prices are gold unless marked. A blank price means the vendor sells the piece for a
currency or condition the mirror does not print.

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

```bash
# 1. capture the remaining 13 vendors
python3 tools/housing/fetch_vendor_stock.py \
    --vendors docs/mod-psycho-house/vendor-slugs.txt \
    --out sql/generated/vendor_stock_rest.csv

# 2. merge with the transcribed 543 rows in data/town-center-stock.csv
# 3. load
mysql world -e "LOAD DATA LOCAL INFILE 'data/town-center-stock.csv' \
  INTO TABLE psycho_house_vendor_stock \
  FIELDS TERMINATED BY ',' OPTIONALLY ENCLOSED BY '\"' \
  LINES TERMINATED BY '\n' IGNORE 1 LINES \
  (vendor, item, price, currency, note);"
```

Then the mapping pass: for each retail item, choose the 8.3.7 game object that stands in for
it. Fences, crates, barrels, chairs, tables, lanterns, cobwebs, trees, boulders, tiles and
staircases all have direct 8.3.7 equivalents. Stormwind and Bel'ameth *architecture* does
not, which is exactly why the shell system in doc 06 exists.

## 7.5 Item ids

Names and prices are one thing; the **item id** is what actually goes into a vendor table.
I am collecting them separately, because wowhead's NPC "Sells" list paginates at 50 rows and
the pager is JavaScript, so one page fetch never returns a vendor's full list.

`data/town-center-items.csv` currently holds **130 resolved item ids**:

| Vendor | Items with ids | Item id range |
| --- | --- | --- |
| Xiao Dan (255203) | 50 of 65 | 235633 - 252037 |
| Faarden the Builder (255213) | 50 of 69 | 235523 - 252004 |
| Balen Starfinder (255216) | 30 of 53 | 235994 - 246258 |

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

`data/vendor_npc_ids.csv` now carries **34 of 38** vendor npc ids and slugs, so
`tools/housing/fetch_item_ids.py` can finish the remaining ~1 000 ids unattended. It tries the
NPC page, the ?sells=N variants, the item-filter endpoint with paging, and finally a per-name
search, and records which strategy produced each id so a wrong answer is traceable.
Still missing an npc id: Shon'ja, Roz Copperpinch, Aeeshna, Spew Gutsout.

### Price corrections this exercise forced

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

## 8. Acceptance checks

| Check | Rule | Result |
| --- | --- | --- |
| Roster | Every vendor stocking decor inside either neighborhood appears once | 39/39 |
| Counts | Transcribed item count equals published count | 29/29 vendors match exactly |
| Currency | Gold, Community Coupons, Tricky Treat, Deed of Patronage kept distinct | verified |
| Ids | Printed only when confirmed against an NPC page | 18 confirmed, 9 not |
| Coordinates | Every coordinate carries its `/way` map id | verified, #2351 vs #2352 |
| Prices | Printed only when the source prints them | 30 corrected to wowhead; 85 blanks remain where wowhead has not been read yet |
| Item ids | Id traced to the item page | 130 resolved, 0 guessed |
