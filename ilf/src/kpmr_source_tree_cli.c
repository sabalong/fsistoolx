/*
 * Command-line interface for building and visualizing KPMR Source trees.
 * Usage: kpmr_source_tree_cli --input <file> [--output <file>] [--format <text|xml>] [--help]
 * SPDX-License-Identifier: MIT
 * Author: Kenny Karnama <kennykarnama@gmail.com>
 */
#include <glib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kpmr_source_tree.h"
#include "otel_cli_c.h"

/**
 * Function to serialize a GNode tree to XML
 */
static xmlNodePtr
serialize_gnode_to_xml(xmlNodePtr parent, GNode *gnode)
{
    if (!gnode)
        return parent;

    if (gnode->data) {
        KpmrSourceData *data = (KpmrSourceData *) gnode->data;

        xmlNodePtr node = xmlNewChild(parent, NULL, BAD_CAST "KpmrSourceNode", NULL);
        xmlNewChild(node, NULL, BAD_CAST "code", BAD_CAST data->code);

        if (data->risk_id) {
            xmlNewChild(node, NULL, BAD_CAST "risk_id", BAD_CAST data->risk_id);
        }
        if (data->assesment_factor) {
            xmlNewChild(node, NULL, BAD_CAST "assesment_factor",
                        BAD_CAST data->assesment_factor);
        }
        if (data->pic) {
            xmlNewChild(node, NULL, BAD_CAST "pic", BAD_CAST data->pic);
        }

        if (data->numeric_id) {
            gchar numeric_id_str[20];
            g_snprintf(numeric_id_str, sizeof(numeric_id_str), "%u", data->numeric_id);
            xmlNewChild(node, NULL, BAD_CAST "numeric_id", BAD_CAST numeric_id_str);
        }

        gchar value_str[20];
            g_snprintf(value_str, sizeof(value_str), "%u", data->value);
            xmlNewChild(node, NULL, BAD_CAST "value", BAD_CAST value_str);

        if (data->remark) {
            xmlNodePtr remarkNode = xmlNewChild(node, NULL, BAD_CAST "remark", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->remark, 
                                                strlen(data->remark));
            xmlAddChild(remarkNode, cdata);
        }
 
        // Recursively serialize children
        GNode *child = gnode->children;
        while (child) {
            serialize_gnode_to_xml(node, child);
            child = child->next;
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
    fprintf(stderr, "KPMR Source Tree Builder CLI\n");
    fprintf(stderr, "============================\n\n");
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
    fprintf(stderr, "  %s --input data/kpmr_source.xml\n\n", prog);
    fprintf(stderr, "  # Print tree structure with text format explicitly\n");
    fprintf(stderr, "  %s --input data/kpmr_source.xml --format text\n\n", prog);
    fprintf(stderr, "  # Save tree structure to XML file\n");
    fprintf(stderr, "  %s --input data/kpmr_source.xml \\\n", prog);
    fprintf(stderr, "        --output kpmr_source_tree.xml \\\n");
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
    fsis_otel_span *parse_span = fsis_otel_start_span(otel, "kpmr_source_tree_cli.parse");
    xmlDocPtr doc = xmlParseFile(input_path);
    if (!doc) {
        fsis_otel_span_error(parse_span, "failed to parse input XML");
        fsis_otel_end_span(parse_span);
        fprintf(stderr, "Error: Failed to parse XML file '%s'\n", input_path);
        xmlCleanupParser();
        return 1;
    }
    fsis_otel_end_span(parse_span);

    // Build the KPMR source tree
    fprintf(stderr, "Building KPMR source tree from '%s'...\n", input_path);
    fsis_otel_span *build_span = fsis_otel_start_span(otel, "kpmr_source_tree_cli.build");
    GNode *root = build_kpmr_source_tree(doc);

    if (!root) {
        fsis_otel_span_error(build_span, "failed to build KPMR source tree");
        fsis_otel_end_span(build_span);
        fprintf(stderr, "Error: Failed to build KPMR source tree\n");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }
    fsis_otel_end_span(build_span);

    int return_code = 0;

    // Output the tree based on format
    if (strcmp(format, "text") == 0) {
        fprintf(stderr, "KPMR Source Tree Structure:\n");
        fprintf(stderr, "===========================\n\n");
        print_kpmr_source_tree(root, 0);
        fprintf(stderr, "\nDone.\n");
    } else if (strcmp(format, "xml") == 0) {
        fsis_otel_span *write_span = fsis_otel_start_span(otel, "kpmr_source_tree_cli.write");
        // Create a new XML document for the tree structure
        xmlDocPtr tree_doc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr tree_root = xmlNewNode(NULL, BAD_CAST "KpmrSourceTree");
        xmlDocSetRootElement(tree_doc, tree_root);

        // Serialize the tree to XML
        serialize_gnode_to_xml(tree_root, root);

        // Write to file
        int save_result = xmlSaveFormatFileEnc(output_path, tree_doc, "UTF-8", 1);
        if (save_result == -1) {
            fsis_otel_span_error(write_span, "failed to write output XML");
            fprintf(stderr, "Error: Failed to save tree to '%s'\n", output_path);
            return_code = 1;
        } else {
            fprintf(stderr, "KPMR source tree saved to '%s'\n", output_path);
        }

        xmlFreeDoc(tree_doc);
        fsis_otel_end_span(write_span);
    }

    // Cleanup
    free_kpmr_source_tree_data(root);
    g_node_destroy(root);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return return_code;
}

int
main(int argc, char **argv)
{
    fsis_otel_runtime *otel = fsis_otel_start("kpmr_source_tree_cli");
    return fsis_otel_finish(otel, run_cli(argc, argv, otel));
}
