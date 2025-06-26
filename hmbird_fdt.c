#include <linux/init.h>
#include <linux/module.h>
#include <linux/libfdt.h>
#include <asm-generic/sections.h>

extern uint8_t __dtb_start[]; // Provided by kernel

static int __init hmbird_fdt_patch(void)
{
    void *fdt = __dtb_start;
    int offset;

    if (fdt_check_header(fdt)) {
        pr_err("hmbird_fdt_patch: Invalid FDT blob\n");
        return -EINVAL;
    }

    // Try multiple paths in case overlay placement varies
    const char *paths[] = {
        "/soc/oplus,hmbird/version_type",
        "/oplus,hmbird/version_type",
        "/platform/soc/oplus,hmbird/version_type",
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

    if (offset < 0) {
        pr_info("hmbird_fdt_patch: oplus,hmbird/version_type not found in FDT\n");
        return -ENODEV;
    }

    const char *val = NULL;
    int len;
    val = fdt_getprop(fdt, offset, "type", &len);
    if (!val || strncmp(val, "HMBIRD_OGKI", len)) {
        pr_info("hmbird_fdt_patch: type property not found or not matching\n");
        return -ENOENT;
    }

    int ret = fdt_setprop_string(fdt, offset, "type", "HMBIRD_GKI");
    if (ret != 0) {
        pr_err("hmbird_fdt_patch: Failed to update FDT property: %s\n", fdt_strerror(ret));
        return ret;
    }

    pr_info("hmbird_fdt_patch: Successfully patched HMBIRD_OGKI → HMBIRD_GKI\n");
    return 0;
}
early_initcall(hmbird_fdt_patch);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fatalcoder524");
MODULE_DESCRIPTION("Patches HMBIRD type in FDT directly.");