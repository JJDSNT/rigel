# ODFileSystem patches

`external/ODFileSystem` tracks upstream
[reinauer/ODFileSystem](https://github.com/reinauer/ODFileSystem) unmodified,
currently pinned at `59bf94f`. Local changes live here as patches.

`scripts/build-odfs.sh` applies them before building; nothing else needs to.

## The set

| Patch | What it does |
| --- | --- |
| `0001-cd01-romtag` | Adds a ROMtag and `platform/amiga/romtag.c`, which registers ODFileSystem in `FileSystem.resource` under DosType `CD01`. |

Without `0001` the handler still builds, is still served from the LIDE board's
second ROM bank, and is still relocated into RAM by the boot loader — it just
never announces itself. `lide.device`'s `FindCDFS()` then finds no CD
filesystem and never mounts `CD0:`, which from the outside looks like the drive
being identified and then ignored: a correct ATAPI exchange through READ
CAPACITY, and no READ TOC or READ(10) ever.

The boot loader finds the tag by scanning the relocated hunk for a Resident
structure and calling its init through `InitResident`, which is why the tag has
to be inside the binary rather than supplied by the host.

## Provenance

From the Bellatrix `legacy` branch, `patches/0012-odfs-cd01-romtag.patch`.
