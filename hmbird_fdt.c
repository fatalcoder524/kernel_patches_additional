#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>

// Internal OF functions (not exported, may fail on some kernels)
extern struct device_node *of_new_node(struct device_node *parent, const char *name);
extern int of_add_property(struct device_node *np, struct property *prop);

// Helper to create the full path and add "type=HMBIRD_GKI"
static struct device_node *create_hmbird_node(void)
{
    struct device_node *soc_np, *oplus_np, *ver_np;
    struct property *prop;

    soc_np = of_find_node_by_path("/soc");
    if (!soc_np) {
        pr_err("hmbird_patch: /soc node not found\n");
        return NULL;
    }

    oplus_np = of_new_node(soc_np, "oplus,hmbird");
    if (!oplus_np) {
        pr_err("hmbird_patch: failed to create /soc/oplus,hmbird\n");
        of_node_put(soc_np);
        return NULL;
    }

    ver_np = of_new_node(oplus_np, "version_type");
    if (!ver_np) {
        pr_err("hmbird_patch: failed to create /soc/oplus,hmbird/version_type\n");
        of_node_put(oplus_np);
        of_node_put(soc_np);
        return NULL;
    }

    // Create and add the 'type' property
    prop = kzalloc(sizeof(*prop), GFP_KERNEL);
    if (!prop) {
        pr_err("hmbird_patch: kmalloc for prop failed\n");
        of_node_put(ver_np);
        return NULL;
    }

    prop->name = kstrdup("type", GFP_KERNEL);
    prop->value = kstrdup("HMBIRD_GKI", GFP_KERNEL);
    prop->length = strlen("HMBIRD_GKI") + 1;

    if (of_add_property(ver_np, prop)) {
        pr_err("hmbird_patch: failed to add 'type' property\n");
        kfree(prop->name);
        kfree(prop->value);
        kfree(prop);
        of_node_put(ver_np);
        return NULL;
    }

    pr_info("hmbird_patch: created /soc/oplus,hmbird/version_type with type=HMBIRD_GKI\n");

    of_node_put(oplus_np);  // We still hold ver_np ref
    of_node_put(soc_np);    // Released

    return ver_np;
}

static int __init hmbird_patch_init(void)
{
    struct device_node *ver_np;
    const char *type;
    int ret;

    ver_np = of_find_node_by_path("/soc/oplus,hmbird/version_type");
    if (!ver_np) {
        ver_np = create_hmbird_node();
        if (!ver_np) {
            pr_err("hmbird_patch: failed to create or find version_type node\n");
            return -ENODEV;
        }
    }

    ret = of_property_read_string(ver_np, "type", &type);
    if (ret) {
        pr_info("hmbird_patch: type property not found\n");
        of_node_put(ver_np);
        return 0;
    }

    if (strcmp(type, "HMBIRD_OGKI")) {
        of_node_put(ver_np);
        return 0;
    }

    struct property *prop = of_find_property(ver_np, "type", NULL);
    if (prop) {
        struct property *new_prop = kmalloc(sizeof(*prop), GFP_KERNEL);
        if (!new_prop) {
            pr_info("hmbird_patch: kmalloc for new_prop failed\n");
            of_node_put(ver_np);
            return 0;
        }
        memcpy(new_prop, prop, sizeof(*prop));
        new_prop->value = kmalloc(strlen("HMBIRD_GKI") + 1, GFP_KERNEL);
        if (!new_prop->value) {
            pr_info("hmbird_patch: kmalloc for new_prop->value failed\n");
            kfree(new_prop);
            of_node_put(ver_np);
            return 0;
        }
        strcpy(new_prop->value, "HMBIRD_GKI");
        new_prop->length = strlen("HMBIRD_GKI") + 1;

        if (of_remove_property(ver_np, prop) != 0) {
            pr_info("hmbird_patch: of_remove_property failed\n");
            return 0;
        }
        if (of_add_property(ver_np, new_prop) != 0) {
            pr_info("hmbird_patch: of_add_property failed\n");
            return 0;
        }
        pr_info("hmbird_patch: success from HMBIRD_OGKI to HMBIRD_GKI\n");
    }
    else {
        pr_info("hmbird_patch: type property structure not found\n");
    }
    of_node_put(ver_np);
    return 0;
}
early_initcall(hmbird_patch_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fatalcoder524");
MODULE_DESCRIPTION("Forcefully convert HMBIRD_OGKI to HMBIRD_GKI, creates node if missing.");