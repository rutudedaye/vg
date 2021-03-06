#include "subcommand.hpp"
#include "../vg.hpp"
#include "../utility.hpp"
#include "xg.hpp"
#include "../algorithms/copy_graph.hpp"
#include "../algorithms/gfa_to_handle.hpp"
#include "../io/save_handle_graph.hpp"
#include <vg/io/stream.hpp>
#include <vg/io/vpkg.hpp>

#include "bdsg/packed_graph.hpp"
#include "bdsg/hash_graph.hpp"
#include "bdsg/odgi.hpp"

#include <unistd.h>
#include <getopt.h>

using namespace vg;
using namespace vg::subcommand;

void help_convert(char** argv) {
    cerr << "usage: " << argv[0] << " convert [options] <input-graph>" << endl
         << "output options:" << endl
         << "    -g, --gfa-in           input in GFA format" << endl
         << "    -v, --vg-out           output in VG format [default]" << endl
         << "    -a, --hash-out         output in HashGraph format" << endl
         << "    -p, --packed-out       output in PackedGraph format" << endl
         << "    -x, --xg-out           output in XG format" << endl
         << "    -o, --odgi-out         output in ODGI format" << endl;
}

int main_convert(int argc, char** argv) {

    string output_format = "vg";
    bool input_gfa = false;

    if (argc == 2) {
        help_convert(argv);
        return 1;
    }

    int c;
    optind = 2; // force optind past command positional argument
    while (true) {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"gfa-in", no_argument, 0, 'g'},
            {"vg-out", no_argument, 0, 'v'},
            {"hash-out", no_argument, 0, 'a'},
            {"packed-out", no_argument, 0, 'p'},
            {"xg-out", no_argument, 0, 'x'},
            {"odgi-out", no_argument, 0, 'o'},
            {0, 0, 0, 0}

        };
        int option_index = 0;
        c = getopt_long (argc, argv, "hgvxapxo",
                long_options, &option_index);

        // Detect the end of the options.
        if (c == -1)
            break;

        switch (c)
        {

        case '?':
        case 'h':
            help_convert(argv);
            return 0;
        case 'g':
            input_gfa = true;
        case 'v':
            output_format = "vg";
            break;
        case 'a':
            output_format = "hash";
            break;
        case 'p':
            output_format = "packed";
            break;
        case 'x':
            output_format = "xg";
            break;
        case 'o':
            output_format = "odgi";
            break;

        default:
            abort();
        }
    }
    
    // allocate a graph using the graph_type string to decide a class
    unique_ptr<HandleGraph> output_graph;
    if (output_format == "vg") {
      output_graph = unique_ptr<HandleGraph>(new VG());
    } else if (output_format == "hash") {
      output_graph = unique_ptr<HandleGraph>(new bdsg::HashGraph());
    } else if (output_format == "packed") {
      output_graph = unique_ptr<HandleGraph>(new bdsg::PackedGraph());
    } else if (output_format == "xg") {
      output_graph = unique_ptr<HandleGraph>(new xg::XG());
    } else if (output_format == "odgi") {
      output_graph = unique_ptr<HandleGraph>(new bdsg::ODGI());
    }
    
    PathHandleGraph* output_path_graph = dynamic_cast<PathHandleGraph*>(output_graph.get());
    
    if (input_gfa) {
        // we have to check this manually since we're not using the istream-based loading
        // functions in order to be able to use the disk-backed loading algorithm
        if (optind >= argc) {
            cerr << "error [vg convert]: no input graph supplied" << endl;
            return 1;
        }
        string input_stream_name = argv[optind];
        if (output_format == "xg") {
            if (input_stream_name == "-") {
                cerr << "error [vg convert]: currently cannot convert GFA from stdin to XG, try loading from disk instead" << endl;
                return 1;
            }
            dynamic_cast<xg::XG*>(output_graph.get())->from_gfa(input_stream_name);
        }
        else {
            try {
                if (output_path_graph != nullptr) {
                    MutablePathMutableHandleGraph* mutable_output_graph = dynamic_cast<MutablePathMutableHandleGraph*>(output_path_graph);
                    assert(mutable_output_graph != nullptr);
                    algorithms::gfa_to_path_handle_graph(input_stream_name, mutable_output_graph,
                                                         input_stream_name != "-", output_format == "odgi");
                }
                else {
                    MutableHandleGraph* mutable_output_graph = dynamic_cast<MutableHandleGraph*>(output_graph.get());
                    assert(mutable_output_graph != nullptr);
                    algorithms::gfa_to_handle_graph(input_stream_name, mutable_output_graph,
                                                    input_stream_name != "-", false);
                }
            } catch (algorithms::GFAFormatError& e) {
                cerr << "error [vg convert]: Input GFA is not acceptable." << endl;
                cerr << e.what() << endl;
                exit(1);
            } catch (std::ios_base::failure& e) {
                cerr << "error [vg convert]: IO error processing input GFA." << endl;
                cerr << e.what() << endl;
                exit(1);
            }
        }
    }
    else {
        unique_ptr<HandleGraph> input_graph;
        get_input_file(optind, argc, argv, [&](istream& in) {
            input_graph = vg::io::VPKG::load_one<HandleGraph>(in);
        });
        
        PathHandleGraph* input_path_graph = dynamic_cast<PathHandleGraph*>(input_graph.get());
        
        if (output_format == "xg") {
            if (input_path_graph != nullptr) {
                dynamic_cast<xg::XG*>(output_graph.get())->from_path_handle_graph(*input_path_graph);
            }
            else {
                dynamic_cast<xg::XG*>(output_graph.get())->from_handle_graph(*input_graph);
            }
        }
        else if (input_path_graph != nullptr && output_path_graph != nullptr) {
            MutablePathMutableHandleGraph* mutable_output_graph = dynamic_cast<MutablePathMutableHandleGraph*>(output_path_graph);
            assert(mutable_output_graph != nullptr);
            // ID hint (currently only for odgi)
            mutable_output_graph->set_id_increment(input_graph->min_node_id());
            algorithms::copy_path_handle_graph(input_path_graph, mutable_output_graph);
        }
        else {
            if (input_path_graph != nullptr) {
                cerr << "warning [vg convert]: output format does not support paths, they are being dopped from the input" << endl;
            }
            MutableHandleGraph* mutable_output_graph = dynamic_cast<MutableHandleGraph*>(output_graph.get());
            assert(mutable_output_graph != nullptr);
            // ID hint (currently only for odgi)
            mutable_output_graph->set_id_increment(input_graph->min_node_id());
            algorithms::copy_handle_graph(input_graph.get(), mutable_output_graph);
        }
    }
    

    // Serialize the graph using VPKG.
    vg::io::save_handle_graph(output_graph.get(), cout);

    return 0;
}

// Register subcommand
static Subcommand vg_convert("convert", "convert graphs between handle-graph compiant formats", main_convert);
