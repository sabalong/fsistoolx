/*
 * Command-line interface for building and visualizing Risk Profile trees.
 * Usage: build_tree_cli --input <file> [--output <file>] [--format <text|xml>] [--help]
 * SPDX-License-Identifier: MIT
 * Author: Kenny Karnama <kennykarnama@gmail.com>
 */
#include <glib.h>
#include<libxml/parser.h>
#include<libxml/tree.h>
#include<libxml/xmlsave.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "risk_profile_tree.h"
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
        RiskProfileData *data = (RiskProfileData *) gnode->data;

        xmlNodePtr node = xmlNewChild(parent, NULL, BAD_CAST "RiskProfileNode", NULL);
        xmlNewChild(node, NULL, BAD_CAST "Profile_ID", BAD_CAST data->profile_id);

        if (data->risiko_name) {
            xmlNewChild(node, NULL, BAD_CAST "risiko_name", BAD_CAST data->risiko_name);
        }

        if (data->faktor_penilaian) {
            xmlNodePtr factorNode = xmlNewChild(node, NULL, BAD_CAST "assessment_factor", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->faktor_penilaian, 
                                                strlen(data->faktor_penilaian));
            xmlAddChild(factorNode, cdata);
        }


        xmlNewChild(node, NULL, BAD_CAST "weight",
                    BAD_CAST g_strdup_printf("%f", data->weight));  
        
        xmlNewChild(node, NULL, BAD_CAST "ID",
                    BAD_CAST g_strdup_printf("%u", data->id));

        xmlNewChild(node, NULL, BAD_CAST "Risk_ID",
                    BAD_CAST g_strdup_printf("%u", data->risk_id));

        if (data->threshold) {
            // xmlNodePtr thresholdNode = 
            // xmlNewChild(node, NULL, BAD_CAST "threshold",
            //         BAD_CAST data->threshold);

            xmlNodePtr thresholdNode = xmlNewChild(node, NULL, BAD_CAST "threshold", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->threshold, 
                                                strlen(data->threshold));
            xmlAddChild(thresholdNode, cdata);
        }

        if (data->scoreFormula) {
            xmlNewChild(node, NULL, BAD_CAST "rating_to_score",
                    BAD_CAST data->scoreFormula);
        }

        if (data->ratingRule) {
            xmlNodePtr ratingRuleNode = xmlNewChild(node, NULL, BAD_CAST "rating_rule", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->ratingRule, 
                                                strlen(data->ratingRule));
            xmlAddChild(ratingRuleNode, cdata);
        }

        if (data->scoreRule) {
            xmlNodePtr scoreRuleNode = xmlNewChild(node, NULL, BAD_CAST "score_rule", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->scoreRule, 
                                                strlen(data->scoreRule));
            xmlAddChild(scoreRuleNode, cdata);
        }

      
        if (data->valueRule) {
            xmlNodePtr valueRuleNode = xmlNewChild(node, NULL, BAD_CAST "value_rule", NULL);
            xmlNodePtr cdata = xmlNewCDataBlock(node->doc, BAD_CAST data->valueRule, 
                                                strlen(data->valueRule));
            xmlAddChild(valueRuleNode, cdata);
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
    fprintf(stderr, "Risk Profile Tree Builder CLI\n");
    fprintf(stderr, "==============================\n\n");
    fprintf(stderr, "Usage: %s [OPTIONS]\n\n", prog);
    fprintf(stderr, "Required Options:\n");
    fprintf(stderr, "  --input <file>             Input XML file containing RiskProfile elements\n\n");
    fprintf(stderr, "Optional Options:\n");
    fprintf(stderr, "  --output <file>            Output file path (only for --format xml)\n");
    fprintf(stderr, "  --format <text|xml>        Output format (default: text)\n");
    fprintf(stderr, "                             - text: Print tree structure to stdout\n");
    fprintf(stderr, "                             - xml: Save tree structure to XML file\n");
    fprintf(stderr, "  --help, -h                 Show this help message\n\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  # Print tree structure to console\n");
    fprintf(stderr, "  %s --input data/risk_profile_report_items.xml\n\n", prog);
    fprintf(stderr, "  # Print tree structure with text format explicitly\n");
    fprintf(stderr, "  %s --input data/risk_profile_report_items.xml --format text\n\n", prog);
    fprintf(stderr, "  # Save tree structure to XML file\n");
    fprintf(stderr, "  %s --input data/risk_profile_report_items.xml \\\n", prog);
    fprintf(stderr, "        --output risk_profile_tree.xml \\\n");
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
    fsis_otel_span *parse_span = fsis_otel_start_span(otel, "build_tree_cli.parse");
    xmlDocPtr doc = xmlParseFile(input_path);
    if (!doc) {
        fsis_otel_span_error(parse_span, "failed to parse input XML");
        fsis_otel_end_span(parse_span);
        fprintf(stderr, "Error: Failed to parse XML file '%s'\n", input_path);
        xmlCleanupParser();
        return 1;
    }
    fsis_otel_end_span(parse_span);

    // Build the risk profile tree
    fprintf(stderr, "Building risk profile tree from '%s'...\n", input_path);
    fsis_otel_span *build_span = fsis_otel_start_span(otel, "build_tree_cli.build");
    GNode *root = build_risk_profile_tree(doc);

    if (!root) {
        fsis_otel_span_error(build_span, "failed to build risk profile tree");
        fsis_otel_end_span(build_span);
        fprintf(stderr, "Error: Failed to build risk profile tree\n");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }
    fsis_otel_end_span(build_span);

    int return_code = 0;

    // Output the tree based on format
    if (strcmp(format, "text") == 0) {
        fprintf(stderr, "Risk Profile Tree Structure:\n");
        fprintf(stderr, "============================\n\n");
        print_risk_profile_tree(root, 0);
        fprintf(stderr, "\nDone.\n");
    } else if (strcmp(format, "xml") == 0) {
        fsis_otel_span *write_span = fsis_otel_start_span(otel, "build_tree_cli.write");
        // Create a new XML document for the tree structure
        xmlDocPtr tree_doc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr tree_root = xmlNewNode(NULL, BAD_CAST "RiskProfileTree");
        xmlDocSetRootElement(tree_doc, tree_root);

        // Add namespace declarations
        xmlNsPtr xsi_ns = xmlNewNs(tree_root, 
                                    BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", 
                                    BAD_CAST "xsi");
        xmlSetNsProp(tree_root, xsi_ns, 
                     BAD_CAST "noNamespaceSchemaLocation", 
                     BAD_CAST "./xsd/InherentRiskProfile.xsd");

        // Serialize the tree to XML
        serialize_gnode_to_xml(tree_root, root);

        // Write to file
        int save_result = xmlSaveFormatFileEnc(output_path, tree_doc, "UTF-8", 1);
        if (save_result == -1) {
            fsis_otel_span_error(write_span, "failed to write output XML");
            fprintf(stderr, "Error: Failed to save tree to '%s'\n", output_path);
            return_code = 1;
        } else {
            fprintf(stderr, "Risk profile tree saved to '%s'\n", output_path);
        }

        xmlFreeDoc(tree_doc);
        fsis_otel_end_span(write_span);
    }

    // Cleanup
    free_risk_profile_tree_data(root);
    g_node_destroy(root);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return return_code;
}

int
main(int argc, char **argv)
{
    fsis_otel_runtime *otel = fsis_otel_start("build_tree_cli");
    return fsis_otel_finish(otel, run_cli(argc, argv, otel));
}
