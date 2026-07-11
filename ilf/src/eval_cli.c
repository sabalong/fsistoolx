/*
 * Command-line interface for evaluating ILF XML files with Risk Profile evaluation.
 * Usage: eval_cli --risk-profile <file> --sandi-source <file> --rating-to-score <file>
 *                 --output <file> [--pic <file>]
 * SPDX-License-Identifier: MIT
 * Author: Kenny Karnama <kennykarnama@gmail.com>
 */
#include <glib.h>
#include<libxml/parser.h>
#include<libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "risk_profile_tree.h"
#include "otel_cli_c.h"

static void
print_usage(const char *prog)
{
    fprintf(stderr, "Risk Profile Evaluation CLI\n");
    fprintf(stderr, "============================\n\n");
    fprintf(stderr, "Usage: %s [OPTIONS]\n\n", prog);
    fprintf(stderr, "Required Options:\n");
    fprintf(stderr, "  --risk-profile <file>      Risk profile report items XML file\n");
    fprintf(stderr, "  --sandi-source <file>      Source data XML file (sandi acuan)\n");
    fprintf(stderr, "  --rating-to-score <file>   Rating to score mapping XML file\n");
    fprintf(stderr, "  --output <file>            Output XML file path\n\n");
    fprintf(stderr, "Optional Options:\n");
    fprintf(stderr,
             "  --pic <file>               Company PIC XML file (defaults to sandi-source)\n");
    fprintf(stderr, "  --help, -h                 Show this help message\n\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s --risk-profile data/risk_profile_report_items.xml \\\n", prog);
    fprintf(stderr, "        --sandi-source data/source_inherent_out.xml \\\n");
    fprintf(stderr, "        --rating-to-score data/rating_to_score.xml \\\n");
    fprintf(stderr, "        --output risk_profile_output.xml\n");
}

static int
run_cli(int argc, char **argv, fsis_otel_runtime *otel)
{
    const char *risk_profile_path = NULL;
    const char *sandi_source_path = NULL;
    const char *rating_to_score_path = NULL;
    const char *output_path = NULL;
    const char *pic_path = NULL;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--risk-profile") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --risk-profile option\n");
                print_usage(argv[0]);
                return 1;
            }
            risk_profile_path = argv[++i];
        } else if (strcmp(argv[i], "--sandi-source") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --sandi-source option\n");
                print_usage(argv[0]);
                return 1;
            }
            sandi_source_path = argv[++i];
        } else if (strcmp(argv[i], "--rating-to-score") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --rating-to-score option\n");
                print_usage(argv[0]);
                return 1;
            }
            rating_to_score_path = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --output option\n");
                print_usage(argv[0]);
                return 1;
            }
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--pic") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --pic option\n");
                print_usage(argv[0]);
                return 1;
            }
            pic_path = argv[++i];
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // Validate required arguments
    if (!risk_profile_path) {
        fprintf(stderr, "Error: Missing required --risk-profile argument\n\n");
        print_usage(argv[0]);
        return 1;
    }
    if (!sandi_source_path) {
        fprintf(stderr, "Error: Missing required --sandi-source argument\n\n");
        print_usage(argv[0]);
        return 1;
    }
    if (!rating_to_score_path) {
        fprintf(stderr, "Error: Missing required --rating-to-score argument\n\n");
        print_usage(argv[0]);
        return 1;
    }
    if (!output_path) {
        fprintf(stderr, "Error: Missing required --output argument\n\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("=== Risk Profile Evaluation Pipeline ===\n");
    printf("Risk Profile:     %s\n", risk_profile_path);
    printf("Sandi Source:     %s\n", sandi_source_path);
    printf("Rating to Score:  %s\n", rating_to_score_path);
    printf("Output:           %s\n", output_path);
    if (pic_path) {
        printf("PIC Data:         %s\n", pic_path);
    }
    printf("\n");

    // Step 1: Parse all XML files
    printf("[1/5] Loading XML files...\n");

    fsis_otel_span *load_span = fsis_otel_start_span(otel, "eval_cli.load");
    xmlDocPtr risk_profile_doc = xmlReadFile(risk_profile_path, NULL, 0);
    if (!risk_profile_doc) {
        fsis_otel_span_error(load_span, "failed to parse risk profile XML");
        fsis_otel_end_span(load_span);
        fprintf(stderr, "Failed to parse risk profile XML file: %s\n", risk_profile_path);
        return 2;
    }

    xmlDocPtr sandi_source_doc = xmlReadFile(sandi_source_path, NULL, 0);
    if (!sandi_source_doc) {
        fsis_otel_span_error(load_span, "failed to parse source XML");
        fsis_otel_end_span(load_span);
        fprintf(stderr, "Failed to parse sandi source XML file: %s\n", sandi_source_path);
        xmlFreeDoc(risk_profile_doc);
        return 2;
    }

    xmlDocPtr rating_to_score_doc = xmlReadFile(rating_to_score_path, NULL, 0);
    if (!rating_to_score_doc) {
        fsis_otel_span_error(load_span, "failed to parse rating mapping XML");
        fsis_otel_end_span(load_span);
        fprintf(stderr, "Failed to parse rating-to-score XML file: %s\n", rating_to_score_path);
        xmlFreeDoc(risk_profile_doc);
        xmlFreeDoc(sandi_source_doc);
        return 2;
    }

    xmlDocPtr pic_doc = NULL;
    if (pic_path) {
        pic_doc = xmlReadFile(pic_path, NULL, 0);
        if (!pic_doc) {
            fsis_otel_span_error(load_span, "failed to parse PIC XML");
            fsis_otel_end_span(load_span);
            fprintf(stderr, "Failed to parse pic XML file: %s\n", pic_path);
            xmlFreeDoc(risk_profile_doc);
            xmlFreeDoc(sandi_source_doc);
            xmlFreeDoc(rating_to_score_doc);
            return 2;
        }
    } else {
        pic_doc = sandi_source_doc; // default to sandi_source when --pic not provided
    }
    fsis_otel_end_span(load_span);

    printf("   ✓ All XML files loaded successfully\n\n");

    // Step 2: Build risk profile tree
    printf("[2/5] Building risk profile tree...\n");

    fsis_otel_span *build_span = fsis_otel_start_span(otel, "eval_cli.build_tree");
    GNode *risk_tree = build_risk_profile_tree(risk_profile_doc);
    if (!risk_tree) {
        fsis_otel_span_error(build_span, "failed to build risk profile tree");
        fsis_otel_end_span(build_span);
        fprintf(stderr, "Failed to build risk profile tree\n");
        xmlFreeDoc(risk_profile_doc);
        xmlFreeDoc(sandi_source_doc);
        xmlFreeDoc(rating_to_score_doc);
        if (pic_doc && pic_doc != sandi_source_doc)
            xmlFreeDoc(pic_doc);
        return 3;
    }
    fsis_otel_end_span(build_span);

    guint node_count = g_node_n_nodes(risk_tree, G_TRAVERSE_ALL);
    printf("   ✓ Built tree with %u nodes\n\n", node_count);

    // Step 3: Evaluate risk profile tree
    printf("[3/5] Evaluating risk profile tree...\n");

    // Prepare evaluation context
    EvalRiskProfileContext ctx = { 0 };
    ctx.risk_profile_doc = risk_profile_doc;
    ctx.sandi_source_doc = sandi_source_doc;

    ILFResult res = { 0 };

    fsis_otel_span *evaluate_span = fsis_otel_start_span(otel, "eval_cli.evaluate");
    ilf_status_t status = evaluate_risk_profile_tree(risk_profile_doc, &ctx, &res);

    if (status != ILF_SUCCESS) {
        fsis_otel_span_error(evaluate_span, "risk profile evaluation failed");
        fsis_otel_end_span(evaluate_span);
        fprintf(stderr, "Evaluation failed: %s\n",
                res.error_message ? res.error_message : "Unknown error");

        // Cleanup
        if (res.error_message)
            free(res.error_message);
        if (res.data) {
            GNode *tree = (GNode *) res.data;
            g_node_destroy(tree);
        }
        g_node_destroy(risk_tree);
        xmlFreeDoc(risk_profile_doc);
        xmlFreeDoc(sandi_source_doc);
        xmlFreeDoc(rating_to_score_doc);
        if (pic_doc && pic_doc != sandi_source_doc)
            xmlFreeDoc(pic_doc);
        xmlCleanupParser();
        return 4;
    }
    fsis_otel_end_span(evaluate_span);

    printf("   ✓ Risk profile evaluation completed\n\n");

    // Step 4: Apply rating-to-score mapping
    printf("[4/5] Applying rating-to-score mapping...\n");

    fsis_otel_span *mapping_span = fsis_otel_start_span(otel, "eval_cli.apply_rating_mapping");
    status = apply_rating_to_score_mapping(risk_profile_doc, rating_to_score_doc);

    if (status != ILF_SUCCESS) {
        fsis_otel_span_error(mapping_span, "rating mapping failed");
        fsis_otel_end_span(mapping_span);
        fprintf(stderr, "Rating-to-score mapping failed\n");

        // Cleanup
        if (res.error_message)
            free(res.error_message);
        if (res.data) {
            GNode *tree = (GNode *) res.data;
            g_node_destroy(tree);
        }
        g_node_destroy(risk_tree);
        xmlFreeDoc(risk_profile_doc);
        xmlFreeDoc(sandi_source_doc);
        xmlFreeDoc(rating_to_score_doc);
        if (pic_doc && pic_doc != sandi_source_doc)
            xmlFreeDoc(pic_doc);
        xmlCleanupParser();
        return 5;
    }
    fsis_otel_end_span(mapping_span);

    printf("   ✓ Rating-to-score mapping completed\n\n");

    // Step 5: Save output
    printf("[5/5] Saving results to %s...\n", output_path);

    fsis_otel_span *write_span = fsis_otel_start_span(otel, "eval_cli.write");
    status = save_risk_profile_result(risk_profile_doc, output_path);

    if (status != ILF_SUCCESS) {
        fsis_otel_span_error(write_span, "failed to write output XML");
        fsis_otel_end_span(write_span);
        fprintf(stderr, "Failed to save output file\n");

        // Cleanup
        if (res.error_message)
            free(res.error_message);
        if (res.data) {
            GNode *tree = (GNode *) res.data;
            g_node_destroy(tree);
        }
        g_node_destroy(risk_tree);
        xmlFreeDoc(risk_profile_doc);
        xmlFreeDoc(sandi_source_doc);
        xmlFreeDoc(rating_to_score_doc);
        if (pic_doc && pic_doc != sandi_source_doc)
            xmlFreeDoc(pic_doc);
        xmlCleanupParser();
        return 6;
    }
    fsis_otel_end_span(write_span);

    printf("   ✓ Output saved successfully\n\n");

    printf("=== Pipeline Completed Successfully ===\n");
    printf("Results saved to: %s\n", output_path);

    // Cleanup
    if (res.error_message)
        free(res.error_message);
    if (res.data) {
        GNode *tree = (GNode *) res.data;
        g_node_destroy(tree);
    }
    g_node_destroy(risk_tree);
    xmlFreeDoc(risk_profile_doc);
    xmlFreeDoc(sandi_source_doc);
    xmlFreeDoc(rating_to_score_doc);
    if (pic_doc && pic_doc != sandi_source_doc)
        xmlFreeDoc(pic_doc);

    xmlCleanupParser();

    return 0;
}

int
main(int argc, char **argv)
{
    fsis_otel_runtime *otel = fsis_otel_start("eval_cli");
    return fsis_otel_finish(otel, run_cli(argc, argv, otel));
}
