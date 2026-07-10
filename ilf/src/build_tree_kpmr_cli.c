/*
 * Command-line interface for building and visualizing KPMR Risk Profile trees.
 * Usage: build_tree_kpmr_cli --input <file> [--output <file>] [--format <text|xml>] [--help]
 * SPDX-License-Identifier: MIT
 * Author: Kenny Karnama <kennykarnama@gmail.com>
 */
#include <glib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <stdio.h>
#include <string.h>
#include "otel_cli_c.h"

// Structure to hold KPMR Risk Profile node data in the tree
typedef struct {
    gchar *profile_id;
    gchar *risiko_name;
    gchar *assessment_factor;
    gchar *derived_from;
    gchar *threshold;
    gchar *input_type;
    gchar *source_reference;
    gchar *calculation_logic;
    gchar *weight;
    gchar *risk_id;
    guint32 id;
    gchar *score_formula;
    gchar *score_rule;
    gchar *rating_rule;
    gchar *value_rule;
    xmlNodePtr xml_node; // Reference to the original XML node
} KpmrRiskProfileData;

/**
 * Create a new KpmrRiskProfileData structure
 */
static KpmrRiskProfileData *
kpmr_risk_profile_data_new(const gchar *profile_id, const gchar *risiko_name,
                           const gchar *assessment_factor,
                           const gchar *derived_from,
                           const gchar *threshold,
                           const gchar *input_type,
                           const gchar *source_reference,
                           const gchar *calculation_logic,
                           const gchar *weight,
                           const gchar *risk_id,
                           const gchar *score_formula,
                           guint32 id, 
                           gchar *score_rule, 
                           gchar *rating_rule, 
                           gchar *value_rule,
                           xmlNodePtr xml_node)
{
    KpmrRiskProfileData *data = g_new0(KpmrRiskProfileData, 1);
    data->profile_id = g_strdup(profile_id);
    data->risiko_name = g_strdup(risiko_name);
    data->assessment_factor = g_strdup(assessment_factor);
    data->derived_from = g_strdup(derived_from);
    data->threshold = g_strdup(threshold);
    data->input_type = g_strdup(input_type);
    data->source_reference = g_strdup(source_reference);
    data->calculation_logic = g_strdup(calculation_logic);
    data->weight = g_strdup(weight);
    data->risk_id = g_strdup(risk_id);
    data->id = id;
    data->score_formula = g_strdup(score_formula);
    data->score_rule = g_strdup(score_rule);
    data->rating_rule = g_strdup(rating_rule);
    data->value_rule = g_strdup(value_rule);
    data->xml_node = xml_node;
    return data;
}

/**
 * Free a KpmrRiskProfileData structure
 */
static void
kpmr_risk_profile_data_free(KpmrRiskProfileData *data)
{
    if (data) {
        g_free(data->profile_id);
        g_free(data->risiko_name);
        g_free(data->assessment_factor);
        g_free(data->derived_from);
        g_free(data->threshold);
        g_free(data->input_type);
        g_free(data->source_reference);
        g_free(data->calculation_logic);
        g_free(data->weight);
        g_free(data->risk_id);
        g_free(data->score_formula);
        g_free(data->score_rule);
        g_free(data->rating_rule);
        g_free(data->value_rule);
        g_free(data);
    }
}

/**
 * Determine if profile_id2 is a child of profile_id1
 * Based on hierarchical matching:
 * - Profile IDs have format: RKxxyyyzzzz where:
 *   - RK = prefix
 *   - xx = risk category (01-12)
 *   - yyy = sub-category (001-999)
 *   - zzzz = specific item (0001-9999)
 * Examples:
 *   - RK01000000 is parent of RK01001000, RK01002000, etc.
 *   - RK01001000 is parent of RK01001001, RK01001002, etc.
 *   - RK10000000 is NOT parent of RK12000000 (different categories)
 */
static gboolean
is_parent_child_relation(const gchar *parent_id, const gchar *child_id)
{
    if (!parent_id || !child_id)
        return FALSE;

    // Both must start with "RK"
    if (strncmp(parent_id, "RK", 2) != 0 || strncmp(child_id, "RK", 2) != 0)
        return FALSE;

    gsize parent_len = strlen(parent_id);
    gsize child_len = strlen(child_id);

    // Both must be 10 characters long (RK + 8 digits)
    if (parent_len != 10 || child_len != 10)
        return FALSE;

    // Cannot be identical
    if (strcmp(parent_id, child_id) == 0)
        return FALSE;

    // Parent must end with at least 3 zeros
    if (strcmp(parent_id + parent_len - 3, "000") != 0)
        return FALSE;

    // Find the level of the parent by counting trailing zeros from the right
    gsize trailing_zeros = 0;
    for (gsize i = parent_len - 1; i >= 2 && parent_id[i] == '0'; i--) {
        trailing_zeros++;
    }

    // Parent must have at least 3 trailing zeros
    if (trailing_zeros < 3)
        return FALSE;

    // Calculate the actual prefix length (non-zero significant part)
    // For RK10000000: trailing_zeros = 6, prefix_len = 10 - 6 = 4 ("RK10")
    // For RK01000000: trailing_zeros = 6, prefix_len = 10 - 6 = 4 ("RK01")
    gsize actual_prefix_len = parent_len - trailing_zeros;

    // Determine hierarchical level based on structure:
    // Format: RK|xx|yyy|zzz where:
    //   xx = 2-digit category (01-12)
    //   yyy = 3-digit subcategory (000-999)
    //   zzz = 3-digit item (000-999)
    // 
    // Level 1: RKxx000000 (category level, 4 chars: RK + 2 category digits)
    // Level 2: RKxxyyyyyy with yyy != 000 (subcategory level)
    // Level 3: RKxxyyyzzzz with zzz != 000 (item level)
    
    // Determine parent's level and required prefix length
    gsize required_prefix_len;
    if (trailing_zeros >= 6) {
        // Parent is at level 1 (e.g., RK01000000 or RK10000000)
        // Must compare first 4 chars (RK + 2-digit category)
        required_prefix_len = 4;
    } else if (trailing_zeros >= 3) {
        // Parent is at level 2 (e.g., RK01001000)
        // Must compare first 7 chars (RK + category + subcategory)
        required_prefix_len = 7;
    } else {
        // Invalid parent level (not enough trailing zeros)
        return FALSE;
    }

    // Use the stricter of actual or required prefix length
    gsize prefix_len = (actual_prefix_len > required_prefix_len) ? actual_prefix_len : required_prefix_len;

    // Child must have EXACT same prefix up to this length
    // This prevents RK10000000 (prefix "RK10") from matching RK12000000 (prefix "RK12")
    if (strncmp(parent_id, child_id, prefix_len) != 0)
        return FALSE;

    // Verify that child has a non-zero digit immediately after parent's prefix
    // This ensures child is exactly one level deeper
    // For example:
    //   Parent: RK01000000 (prefix_len=4: "RK01")
    //   Valid child: RK01001000 (has '001' after "RK01")
    //   Invalid child: RK01000001 (has '000' after "RK01", then '0001')
    
    // Check the next segment after parent's prefix
    gsize segment_start = prefix_len;
    gsize segment_len;
    
    // Determine segment length based on parent level
    // Looking at the structure: RKxx|yyy|zzz where xx=category, yyy=subcategory, zzz=item
    if (trailing_zeros >= 6) {
        segment_len = 3; // Level 1 → 2: next 3 digits (subcategory)
    } else if (trailing_zeros >= 3) {
        segment_len = 3; // Level 2 → 3: next 3 digits (item)
    } else {
        // Not enough trailing zeros for valid parent
        return FALSE;
    }
    
    // Make sure we don't go out of bounds
    if (segment_start + segment_len > child_len)
        return FALSE;
    
    // The segment immediately after parent's prefix must NOT be all zeros
    gboolean has_non_zero = FALSE;
    for (gsize i = segment_start; i < segment_start + segment_len; i++) {
        if (child_id[i] != '0') {
            has_non_zero = TRUE;
            break;
        }
    }
    
    if (!has_non_zero)
        return FALSE;

    // Additionally verify that remaining part after the immediate segment is all zeros
    // This ensures direct parent-child relationship (not grandparent-grandchild)
    for (gsize i = segment_start + segment_len; i < child_len; i++) {
        if (child_id[i] != '0')
            return FALSE;
    }

    return TRUE;
}

/**
 * Find a node in the tree by Profile_ID
 */
static GNode *
find_node_by_profile_id(GNode *root, const gchar *profile_id)
{
    if (!root || !profile_id)
        return NULL;

    if (root->data) {
        KpmrRiskProfileData *data = (KpmrRiskProfileData *) root->data;
        if (data->profile_id && strcmp(data->profile_id, profile_id) == 0)
            return root;
    }

    // Search in children
    for (GNode *child = root->children; child; child = child->next) {
        GNode *found = find_node_by_profile_id(child, profile_id);
        if (found)
            return found;
    }

    return NULL;
}

/**
 * Build a GNode tree from KPMR risk profile XML document
 */
static GNode *
build_kpmr_risk_profile_tree(xmlDocPtr doc)
{
    if (!doc)
        return NULL;

    xmlNodePtr root_element = xmlDocGetRootElement(doc);
    if (!root_element)
        return NULL;

    // Create virtual root
    GNode *tree_root = g_node_new(NULL);

    // Collect all DATA_RECORD elements
    GPtrArray *all_profiles = g_ptr_array_new();

    for (xmlNodePtr node = root_element->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE &&
            xmlStrcmp(node->name, BAD_CAST "DATA_RECORD") == 0) {

            xmlChar *profile_id = NULL;
            xmlChar *risiko_name = NULL;
            xmlChar *assessment_factor = NULL;
            xmlChar *derived_from = NULL;
            xmlChar *threshold = NULL;
            xmlChar *input_type = NULL;
            xmlChar *source_reference = NULL;
            xmlChar *calculation_logic = NULL;
            xmlChar *weight = NULL;
            xmlChar *risk_id_str = NULL;
            xmlChar *score_formula = NULL;
            xmlChar *id_str = NULL;
            xmlChar *rating_rule = NULL;
            xmlChar *score_rule = NULL;
            xmlChar *value_rule = NULL;

            for (xmlNodePtr child = node->children; child; child = child->next) {
                if (child->type == XML_ELEMENT_NODE) {
                    xmlChar *content = xmlNodeGetContent(child);
                    if (xmlStrcmp(child->name, BAD_CAST "profile_id") == 0) {
                        profile_id = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "risiko_name") == 0) {
                        risiko_name = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "assessment_factor") == 0) {
                        assessment_factor = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "derived_from") == 0) {
                        derived_from = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "threshold") == 0) {
                        threshold = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "input_type") == 0) {
                        input_type = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "source_reference") == 0) {
                        source_reference = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "calculation_logic") == 0) {
                        calculation_logic = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "weight") == 0) {
                        weight = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "risk_id") == 0) {
                        risk_id_str = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "score_formula") == 0) {
                        score_formula = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "id") == 0) {
                        id_str = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "rating_rule") == 0) {
                        rating_rule = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "score_rule") == 0) {
                        score_rule = content;
                    } else if (xmlStrcmp(child->name, BAD_CAST "value_rule") == 0) {
                        value_rule = content;
                    } else {
                        xmlFree(content);
                    }
                }
            }

            // Process all records including those with empty Profile_ID
            guint32 id = 0;
            if (id_str)
                id = (guint32) atoi((const char *) id_str);

            KpmrRiskProfileData *data = kpmr_risk_profile_data_new(
                (const gchar *) profile_id,
                (const gchar *) risiko_name,
                (const gchar *) assessment_factor,
                (const gchar *) derived_from,
                (const gchar *) threshold,
                (const gchar *) input_type,
                (const gchar *) source_reference,
                (const gchar *) calculation_logic,
                (const gchar *) weight,
                (const gchar *) risk_id_str,
                (const gchar *) score_formula,
                id,
                (gchar *) score_rule,
                (gchar *) rating_rule,
                (gchar *) value_rule,
                node
            );

            g_ptr_array_add(all_profiles, data);

            if (profile_id) xmlFree(profile_id);
            if (risiko_name) xmlFree(risiko_name);
            if (assessment_factor) xmlFree(assessment_factor);
            if (derived_from) xmlFree(derived_from);
            if (threshold) xmlFree(threshold);
            if (input_type) xmlFree(input_type);
            if (source_reference) xmlFree(source_reference);
            if (calculation_logic) xmlFree(calculation_logic);
            if (weight) xmlFree(weight);
            if (risk_id_str) xmlFree(risk_id_str);
            if (score_formula) xmlFree(score_formula);
            if (score_rule) xmlFree(score_rule);
            if (rating_rule) xmlFree(rating_rule);
            if (value_rule) xmlFree(value_rule);
            if (id_str) xmlFree(id_str);
        }
    }

    // First pass: Create all nodes and store in hash table for quick lookup
    GHashTable *node_map = g_hash_table_new(g_str_hash, g_str_equal);
    
    for (guint i = 0; i < all_profiles->len; i++) {
        KpmrRiskProfileData *data = g_ptr_array_index(all_profiles, i);
        GNode *new_node = g_node_new(data);
        
        if (data->profile_id && strlen(data->profile_id) > 0) {
            g_hash_table_insert(node_map, data->profile_id, new_node);
        }
    }
    
    // Second pass: Build parent-child relationships
    for (guint i = 0; i < all_profiles->len; i++) {
        KpmrRiskProfileData *data = g_ptr_array_index(all_profiles, i);
        
        if (!data->profile_id || strlen(data->profile_id) == 0) {
            continue;  // Skip empty profile_id entries
        }
        
        GNode *current_node = g_hash_table_lookup(node_map, data->profile_id);
        if (!current_node) continue;
        
        // Skip if node already has a parent (already attached to tree)
        if (current_node->parent != NULL) continue;
        
        // Try to find parent
        GNode *parent_node = NULL;
        for (guint j = 0; j < all_profiles->len; j++) {
            if (i == j) continue;
            
            KpmrRiskProfileData *potential_parent = g_ptr_array_index(all_profiles, j);
            if (is_parent_child_relation(potential_parent->profile_id, data->profile_id)) {
                parent_node = g_hash_table_lookup(node_map, potential_parent->profile_id);
                if (parent_node)
                    break;
            }
        }
        
        if (parent_node) {
            g_node_append(parent_node, current_node);
        } else {
            g_node_append(tree_root, current_node);
        }
    }
    
    g_hash_table_destroy(node_map);

    g_ptr_array_free(all_profiles, TRUE);

    return tree_root;
}

/**
 * Print the tree structure (for debugging)
 */
static void
print_kpmr_risk_profile_tree(GNode *node, gint depth)
{
    if (!node)
        return;

    if (node->data) {
        KpmrRiskProfileData *data = (KpmrRiskProfileData *) node->data;

        // Print indentation
        for (gint i = 0; i < depth; i++)
            fprintf(stdout, "  ");

        fprintf(stdout, "[%s] %s", data->profile_id ? data->profile_id : "N/A",
                data->risiko_name ? data->risiko_name : "N/A");

        if (data->assessment_factor && strlen(data->assessment_factor) > 0) {
            fprintf(stdout, " - %s", data->assessment_factor);
        }

        fprintf(stdout, "\n");
    }

    // Recursively print children
    for (GNode *child = node->children; child; child = child->next) {
        print_kpmr_risk_profile_tree(child, depth + 1);
    }
}

/**
 * Free all data in the tree
 */
static gboolean
free_node_data(GNode *node, gpointer data)
{
    (void) data; // Unused
    if (node->data) {
        kpmr_risk_profile_data_free((KpmrRiskProfileData *) node->data);
        node->data = NULL;
    }
    return FALSE;
}

static void
free_kpmr_risk_profile_tree_data(GNode *root)
{
    if (root)
        g_node_traverse(root, G_IN_ORDER, G_TRAVERSE_ALL, -1, free_node_data, NULL);
}

/**
 * Function to serialize a GNode tree to XML
 */
static xmlNodePtr
serialize_gnode_to_xml(xmlNodePtr parent, GNode *gnode)
{
    if (!gnode)
        return parent;

    if (gnode->data) {
        KpmrRiskProfileData *data = (KpmrRiskProfileData *) gnode->data;

        xmlNodePtr node = xmlNewChild(parent, NULL, BAD_CAST "node", NULL);

        if (data->risiko_name)
            xmlNewChild(node, NULL, BAD_CAST "risiko_name", BAD_CAST data->risiko_name);
        if (data->profile_id)
            xmlNewChild(node, NULL, BAD_CAST "profile_id", BAD_CAST data->profile_id);
        if (data->assessment_factor) {
            xmlNodePtr factorNode = xmlNewChild(node, NULL, BAD_CAST "assessment_factor", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->assessment_factor, 
                                                strlen(data->assessment_factor));
            xmlAddChild(factorNode, cdata);
        }
        if (data->derived_from)
            xmlNewChild(node, NULL, BAD_CAST "derived_from", BAD_CAST data->derived_from);
        if (data->threshold)
            xmlNewChild(node, NULL, BAD_CAST "threshold", BAD_CAST data->threshold);
        if (data->input_type)
            xmlNewChild(node, NULL, BAD_CAST "input_type", BAD_CAST data->input_type);
        if (data->source_reference)
            xmlNewChild(node, NULL, BAD_CAST "source_reference", BAD_CAST data->source_reference);
        if (data->calculation_logic)
            xmlNewChild(node, NULL, BAD_CAST "calculation_logic", BAD_CAST data->calculation_logic);
        if (data->risk_id)
            xmlNewChild(node, NULL, BAD_CAST "risk_id", BAD_CAST data->risk_id);
        if (data->id) {
            gchar id_str[20];
            g_snprintf(id_str, sizeof(id_str), "%u", data->id);
            xmlNewChild(node, NULL, BAD_CAST "id", BAD_CAST id_str);
        }
        if (data->weight)
            xmlNewChild(node, NULL, BAD_CAST "weight", BAD_CAST data->weight);
        if (data->score_formula)
            xmlNewChild(node, NULL, BAD_CAST "score_formula", BAD_CAST data->score_formula);
         
            if (data->score_formula) {
            xmlNodePtr ratingToScoreNode = xmlNewChild(node, NULL, BAD_CAST "rating_to_score", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->score_formula, 
                                                strlen(data->score_formula));
            xmlAddChild(ratingToScoreNode, cdata);
        }

        if (data->rating_rule) {
            xmlNodePtr ratingRuleNode = xmlNewChild(node, NULL, BAD_CAST "rating_rule", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->rating_rule, 
                                                strlen(data->rating_rule));
            xmlAddChild(ratingRuleNode, cdata);
        }

        if (data->score_rule) {
            xmlNodePtr scoreRuleNode = xmlNewChild(node, NULL, BAD_CAST "score_rule", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->score_rule, 
                                                strlen(data->score_rule));
            xmlAddChild(scoreRuleNode, cdata);
        }

      
        if (data->value_rule) {
            xmlNodePtr valueRuleNode = xmlNewChild(node, NULL, BAD_CAST "value_rule", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->value_rule, 
                                                strlen(data->value_rule));
            xmlAddChild(valueRuleNode, cdata);
        }


        // Recursively serialize children under <children>
        if (gnode->children) {
            xmlNodePtr children = xmlNewChild(node, NULL, BAD_CAST "children", NULL);
            GNode *child = gnode->children;
            while (child) {
                serialize_gnode_to_xml(children, child);
                child = child->next;
            }
        }

        return node;
    } else {
        // Virtual root - serialize children directly
        GNode *child = gnode->children;
        while (child) {
            serialize_gnode_to_xml(parent, child);
            child = child->next;
        }
    }

    return parent;
}

static void
print_usage(const char *prog)
{
    fprintf(stderr, "KPMR Risk Profile Tree Builder CLI\n");
    fprintf(stderr, "===================================\n\n");
    fprintf(stderr, "Usage: %s [OPTIONS]\n\n", prog);
    fprintf(stderr, "Required Options:\n");
    fprintf(stderr, "  --input <file>             Input XML file containing DATA_RECORD elements\n\n");
    fprintf(stderr, "Optional Options:\n");
    fprintf(stderr, "  --output <file>            Output file path (only for --format xml)\n");
    fprintf(stderr, "  --format <text|xml>        Output format (default: text)\n");
    fprintf(stderr, "                             - text: Print tree structure to stdout\n");
    fprintf(stderr, "                             - xml: Save tree structure to XML file\n");
    fprintf(stderr, "  --help, -h                 Show this help message\n\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  # Print tree structure to console\n");
    fprintf(stderr, "  %s --input data/kpmr_risk_profile.xml\n\n", prog);
    fprintf(stderr, "  # Print tree structure with text format explicitly\n");
    fprintf(stderr, "  %s --input data/kpmr_risk_profile.xml --format text\n\n", prog);
    fprintf(stderr, "  # Save tree structure to XML file\n");
    fprintf(stderr, "  %s --input data/kpmr_risk_profile.xml \\\n", prog);
    fprintf(stderr, "        --output kpmr_risk_profile_tree.xml \\\n");
    fprintf(stderr, "        --format xml\n");
}

static int
run_cli(int argc, char **argv, fsis_otel_runtime *otel)
{
    const char *input_path = NULL;
    const char *output_path = NULL;
    const char *format = "text";

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--input") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing value for --input option\n");
                print_usage(argv[0]);
                return 1;
            }
            input_path = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing value for --output option\n");
                print_usage(argv[0]);
                return 1;
            }
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--format") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing value for --format option\n");
                print_usage(argv[0]);
                return 1;
            }
            format = argv[++i];
        } else {
            fprintf(stderr, "Error: Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // Validate required options
    if (!input_path) {
        fprintf(stderr, "Error: --input is required\n");
        print_usage(argv[0]);
        return 1;
    }

    // Validate format option
    if (strcmp(format, "text") != 0 && strcmp(format, "xml") != 0) {
        fprintf(stderr, "Error: Invalid format '%s'. Must be 'text' or 'xml'\n", format);
        print_usage(argv[0]);
        return 1;
    }

    // For XML format, output path is required
    if (strcmp(format, "xml") == 0 && !output_path) {
        fprintf(stderr, "Error: --output is required when using --format xml\n");
        print_usage(argv[0]);
        return 1;
    }

    // Initialize XML library
    xmlInitParser();

    // Parse input XML file
    fsis_otel_span *parse_span = fsis_otel_start_span(otel, "build_tree_kpmr_cli.parse");
    xmlDocPtr doc = xmlParseFile(input_path);
    if (!doc) {
        fsis_otel_span_error(parse_span, "failed to parse input XML");
        fsis_otel_end_span(parse_span);
        fprintf(stderr, "Error: Failed to parse XML file '%s'\n", input_path);
        xmlCleanupParser();
        return 1;
    }
    fsis_otel_end_span(parse_span);

    // Build the KPMR risk profile tree
    fprintf(stderr, "Building KPMR risk profile tree from '%s'...\n", input_path);
    fsis_otel_span *build_span = fsis_otel_start_span(otel, "build_tree_kpmr_cli.build");
    GNode *root = build_kpmr_risk_profile_tree(doc);

    if (!root) {
        fsis_otel_span_error(build_span, "failed to build KPMR risk profile tree");
        fsis_otel_end_span(build_span);
        fprintf(stderr, "Error: Failed to build KPMR risk profile tree\n");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }
    fsis_otel_end_span(build_span);

    int return_code = 0;

    // Output the tree based on format
    if (strcmp(format, "text") == 0) {
        fprintf(stderr, "KPMR Risk Profile Tree Structure:\n");
        fprintf(stderr, "==================================\n\n");
        print_kpmr_risk_profile_tree(root, 0);
        fprintf(stderr, "\nDone.\n");
    } else if (strcmp(format, "xml") == 0) {
        fsis_otel_span *write_span = fsis_otel_start_span(otel, "build_tree_kpmr_cli.write");
        // Create a new XML document for the tree structure
        xmlDocPtr tree_doc = xmlNewDoc(BAD_CAST "1.0");
        
        // Create root element with KPMR namespace
        xmlNsPtr kpmr_ns = NULL;
        xmlNodePtr tree_root = xmlNewNode(kpmr_ns, BAD_CAST "kpmr_risk_profile_tree");
        xmlDocSetRootElement(tree_doc, tree_root);
        
        // Set default namespace
        kpmr_ns = xmlNewNs(tree_root, 
                           BAD_CAST "http://example.com/kpmr", 
                           NULL);
        xmlSetNs(tree_root, kpmr_ns);
        
        // Add xsi namespace and schemaLocation
        xmlNsPtr xsi_ns = xmlNewNs(tree_root, 
                                    BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", 
                                    BAD_CAST "xsi");
        xmlSetNsProp(tree_root, xsi_ns, 
                     BAD_CAST "schemaLocation", 
                     BAD_CAST "http://example.com/kpmr ./xsd/KPMRRiskProfile.xsd");

        // Serialize the tree to XML
        serialize_gnode_to_xml(tree_root, root);

        // Write to file
        int save_result = xmlSaveFormatFileEnc(output_path, tree_doc, "UTF-8", 1);
        if (save_result == -1) {
            fsis_otel_span_error(write_span, "failed to write output XML");
            fprintf(stderr, "Error: Failed to save tree to '%s'\n", output_path);
            return_code = 1;
        } else {
            fprintf(stderr, "KPMR risk profile tree saved to '%s'\n", output_path);
        }

        xmlFreeDoc(tree_doc);
        fsis_otel_end_span(write_span);
    }

    // Cleanup
    free_kpmr_risk_profile_tree_data(root);
    g_node_destroy(root);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return return_code;
}

int
main(int argc, char **argv)
{
    fsis_otel_runtime *otel = fsis_otel_start("build_tree_kpmr_cli");
    return fsis_otel_finish(otel, run_cli(argc, argv, otel));
}
