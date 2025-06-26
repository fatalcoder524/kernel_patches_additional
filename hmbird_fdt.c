#include <linux/init.h>
#include <linux/module.h>
#include <linux/libfdt.h>
#include <asm-generic/sections.h>

extern uint8_t __dtb_start[]; // Provided by linker

static int __init hmbird_fdt_patch(void)
{
    void *fdt = __dtb_start;
    int offset;

    if (fdt_check_header(fdt)) {
        pr_err("hmbird_fdt_patch: Invalid FDT blob\n");
        return -EINVAL;
    }

    const char *paths[] = {
        "/soc/oplus,hmbird/version_type",
        "/oplus,hmbird/version_type",  // May still exist without /soc prefix
        NULL
    };

    int i;
    for (i = 0; paths[i]; i++) {
        offset = fdt_path_offset(fdt, paths[i]);
        if (offset >= 0) {
            pr_info("hmbird_fdt_patch: Found node at %s\n", paths[i]);
            break;
        }
    }

    // Step 1: Try known paths
    if (offset < 0) {
        pr_info("hmbird_fdt_patch: oplus,hmbird/version_type not found, searching under /soc...\n");

        int soc_off = fdt_path_offset(fdt, "/soc");
        if (soc_off < 0) {
            pr_info("hmbird_fdt_patch: /soc not found, skipping creation\n");
            return -ENODEV;
        }

        offset = fdt_subnode_offset(fdt, soc_off, "oplus,hmbird");
        if (offset >= 0) {
            offset = fdt_subnode_offset(fdt, offset, "version_type");
        }

        if (offset < 0) {
            pr_info("hmbird_fdt_patch: Creating /soc/oplus,hmbird/version_type\n");

            int hmbird_off = fdt_add_subnode(fdt, soc_off, "oplus,hmbird");
            if (hmbird_off < 0) {
                pr_err("hmbird_fdt_patch: Failed to create /soc/oplus,hmbird (%d)\n", hmbird_off);
                return hmbird_off;
            }

            offset = fdt_add_subnode(fdt, hmbird_off, "version_type");
            if (offset < 0) {
                pr_err("hmbird_fdt_patch: Failed to create version_type (%d)\n", offset);
                return offset;
            }
        }
    }

    // Step 2: Set the type property
    int ret = fdt_setprop_string(fdt, offset, "type", "HMBIRD_GKI");
    if (ret != 0) {
        pr_err("hmbird_fdt_patch: Failed to update 'type' property: %s\n", fdt_strerror(ret));
        return ret;
    }

    pr_info("hmbird_fdt_patch: Successfully patched HMBIRD_OGKI → HMBIRD_GKI\n");
    return 0;
}
early_initcall(hmbird_fdt_patch);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fatalcoder524");
MODULE_DESCRIPTION("Patches /soc/oplus,hmbird/version_type.type to HMBIRD_GKI. Creates if missing.");