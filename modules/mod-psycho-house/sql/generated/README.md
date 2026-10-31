# Generated SQL output

`tools/housing/harvest_decor.py` writes a dated `housing_decor_<date>.sql` here when it
runs against a live world database. Nothing in this folder is hand written: the file is
regenerated from `gameobject_template` plus the curated family table, and it is the only
place where a spawn decision is automated.

The files that ship with the module live in `drafts/*/sql/world/` and are committed by
hand after a review pass.
