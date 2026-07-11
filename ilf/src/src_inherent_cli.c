/*
 * Command-line interface for evaluating source inherent and konsolidasi logic.
 * This CLI calls evaluate_src_inherent and evaluate_konsolidasi_inherent functions.
 * Usage: src_inherent_cli --input <file> --pic <file> [--output-pic <file>] [--output-doc <file>]
 * SPDX-License-Identifier: MIT
 * Author: Kenny Karnama <kennykarnama@gmail.com>
 */
#include <glib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "hdr.h"
#include "otel_cli_c.h"

static void
print_usage(const char *prog)
{
    fprintf(stderr, "Source Inherent Evaluation CLI\n");
    fprintf(stderr, "================================\n\n");
    fprintf(stderr, "Usage: %s [OPTIONS]\n\n", prog);
    fprintf(stderr, "Required Options:\n");
    fprintf(stderr, "  --input <file>             Source inherent XML file (sourceInherentRevampReport)\n");
    fprintf(stderr, "  --pic <file>               Company PIC XML file (prmtr_company)\n\n");
    fprintf(stderr, "Optional Options:\n");
    fprintf(stderr, "  --output-pic <file>        Output PIC file path (default: updated_pic.xml)\n");
    fprintf(stderr, "  --output-doc <file>        Output document file path (default: updated_doc.xml)\n");
    fprintf(stderr, "  --skip-konsolidasi         Skip konsolidasi inherent evaluation\n");
    fprintf(stderr, "  --help, -h                 Show this help message\n\n");
    fprintf(stderr, "Description:\n");
    fprintf(stderr, "  This CLI evaluates source inherent logic by:\n");
    fprintf(stderr, "  1. Parsing the input XML file containing sourceInherentRevampReport records\n");
    fprintf(stderr, "  2. Evaluating logicLjk for each record using TCC (Tiny C Compiler)\n");
    fprintf(stderr, "  3. Updating the PIC document with evaluation results\n");
    fprintf(stderr, "  4. Optionally evaluating logicKonsolidasi for consolidation logic\n\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  # Evaluate source inherent and konsolidasi\n");
    fprintf(stderr, "  %s --input data/inherent_source.xml \\\n", prog);
    fprintf(stderr, "        --pic data/prmtr_company.xml\n\n");
    fprintf(stderr, "  # Evaluate source inherent only (skip konsolidasi)\n");
    fprintf(stderr, "  %s --input data/inherent_source.xml \\\n", prog);
    fprintf(stderr, "        --pic data/prmtr_company.xml \\\n");
    fprintf(stderr, "        --skip-konsolidasi\n\n");
    fprintf(stderr, "  # Custom output file paths\n");
    fprintf(stderr, "  %s --input data/inherent_source.xml \\\n", prog);
    fprintf(stderr, "        --pic data/prmtr_company.xml \\\n");
    fprintf(stderr, "        --output-pic results/pic_output.xml \\\n");
    fprintf(stderr, "        --output-doc results/doc_output.xml\n");
}

static int
run_cli(int argc, char **argv, fsis_otel_runtime *otel)
{
    const char *input_path = NULL;
    const char *pic_path = NULL;
    const char *output_pic_path = "updated_pic.xml";
    const char *output_doc_path = "updated_doc.xml";
    gboolean skip_konsolidasi = FALSE;

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
        } else if (strcmp(argv[i], "--pic") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing value for --pic option\n");
                print_usage(argv[0]);
                return 1;
            }
            pic_path = argv[++i];
        } else if (strcmp(argv[i], "--output-pic") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing value for --output-pic option\n");
                print_usage(argv[0]);
                return 1;
            }
            output_pic_path = argv[++i];
        } else if (strcmp(argv[i], "--output-doc") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing value for --output-doc option\n");
                print_usage(argv[0]);
                return 1;
            }
            output_doc_path = argv[++i];
        } else if (strcmp(argv[i], "--skip-konsolidasi") == 0) {
            skip_konsolidasi = TRUE;
        } else {
            fprintf(stderr, "Error: Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // Validate required arguments
    if (!input_path) {
        fprintf(stderr, "Error: --input option is required\n");
        print_usage(argv[0]);
        return 1;
    }

    if (!pic_path) {
        fprintf(stderr, "Error: --pic option is required\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("=== Source Inherent Evaluation CLI ===\n");
    printf("Input file:      %s\n", input_path);
    printf("PIC file:        %s\n", pic_path);
    printf("Output PIC:      %s\n", output_pic_path);
    printf("Output Doc:      %s\n", output_doc_path);
    printf("Skip Konsolidasi: %s\n\n", skip_konsolidasi ? "Yes" : "No");

    // Initialize libxml2
    LIBXML_TEST_VERSION

    // Parse input XML file
    printf("1. Parsing input XML file...\n");
    fsis_otel_span *load_span = fsis_otel_start_span(otel, "src_inherent_cli.load");
    xmlDocPtr input_doc = xmlParseFile(input_path);
    if (!input_doc) {
        fsis_otel_span_error(load_span, "failed to parse input XML");
        fsis_otel_end_span(load_span);
        fprintf(stderr, "Error: Failed to parse input file: %s\n", input_path);
        xmlCleanupParser();
        return 1;
    }
    printf("   ✓ Input file parsed successfully\n");

    // Parse PIC XML file
    printf("2. Parsing PIC XML file...\n");
    xmlDocPtr pic_doc = xmlParseFile(pic_path);
    if (!pic_doc) {
        fsis_otel_span_error(load_span, "failed to parse PIC XML");
        fsis_otel_end_span(load_span);
        fprintf(stderr, "Error: Failed to parse PIC file: %s\n", pic_path);
        xmlFreeDoc(input_doc);
        xmlCleanupParser();
        return 1;
    }
    fsis_otel_end_span(load_span);
    printf("   ✓ PIC file parsed successfully\n");

    // Create evaluation context
    printf("3. Setting up evaluation context...\n");
    EvalContext eval_ctx;
    eval_ctx.pic_doc = pic_doc;
    eval_ctx.memo = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    printf("   ✓ Evaluation context created\n");

    // Evaluate source inherent
    printf("4. Evaluating source inherent logic (logicLjk)...\n");
    ILFResult src_result;
    fsis_otel_span *evaluate_span = fsis_otel_start_span(otel, "src_inherent_cli.evaluate_source");
    ilf_status_t status = evaluate_src_inherent(input_doc, &eval_ctx, &src_result);

    if (status != ILF_SUCCESS) {
        fsis_otel_span_error(evaluate_span, "source inherent evaluation failed");
        fsis_otel_end_span(evaluate_span);
        fprintf(stderr, "Error: Source inherent evaluation failed\n");
        if (src_result.error_message) {
            fprintf(stderr, "   Error message: %s\n", src_result.error_message);
            g_free(src_result.error_message);
        }
        g_hash_table_destroy(eval_ctx.memo);
        xmlFreeDoc(pic_doc);
        xmlFreeDoc(input_doc);
        xmlCleanupParser();
        return 1;
    }
    fsis_otel_end_span(evaluate_span);
    printf("   ✓ Source inherent evaluation completed\n");
    printf("   ✓ Updated PIC document saved to: %s\n", output_pic_path);

    // Evaluate konsolidasi inherent (if not skipped)
    if (!skip_konsolidasi) {
        printf("5. Evaluating konsolidasi inherent logic (logicKonsolidasi)...\n");
        ILFResult kons_result;
        fsis_otel_span *consolidate_span = fsis_otel_start_span(otel, "src_inherent_cli.evaluate_consolidation");
        status = evaluate_konsolidasi_inherent(input_doc, &eval_ctx, &kons_result);

        if (status != ILF_SUCCESS) {
            fsis_otel_span_error(consolidate_span, "consolidation evaluation failed");
            fsis_otel_end_span(consolidate_span);
            fprintf(stderr, "Error: Konsolidasi inherent evaluation failed\n");
            if (kons_result.error_message) {
                fprintf(stderr, "   Error message: %s\n", kons_result.error_message);
                g_free(kons_result.error_message);
            }
            g_hash_table_destroy(eval_ctx.memo);
            xmlFreeDoc(pic_doc);
            xmlFreeDoc(input_doc);
            xmlCleanupParser();
            return 1;
        }
        fsis_otel_end_span(consolidate_span);
        printf("   ✓ Konsolidasi inherent evaluation completed\n");
        printf("   ✓ Updated document saved to: %s\n", output_doc_path);
    } else {
        printf("5. Skipping konsolidasi inherent evaluation (--skip-konsolidasi)\n");
    }

    // Cleanup
    printf("\n6. Cleaning up resources...\n");
    g_hash_table_destroy(eval_ctx.memo);
    xmlFreeDoc(pic_doc);
    xmlFreeDoc(input_doc);
    xmlCleanupParser();
    printf("   ✓ Cleanup complete\n");

    printf("\n=== Evaluation completed successfully ===\n");
    printf("Output files:\n");
    printf("  - PIC document: %s\n", output_pic_path);
    if (!skip_konsolidasi) {
        printf("  - Updated document: %s\n", output_doc_path);
    }

    return 0;
}

int
main(int argc, char **argv)
{
    fsis_otel_runtime *otel = fsis_otel_start("src_inherent_cli");
    return fsis_otel_finish(otel, run_cli(argc, argv, otel));
}
