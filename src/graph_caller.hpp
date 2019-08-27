#ifndef VG_GRAPH_CALLER_HPP_INCLUDED
#define VG_GRAPH_CALLER_HPP_INCLUDED

#include <iostream>
#include <algorithm>
#include <functional>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <tuple>
#include "handle.hpp"
#include "snarls.hpp"
#include "traversal_finder.hpp"
#include "snarl_caller.hpp"
#include "region.hpp"

namespace vg {

using namespace std;

/**
 * GraphCaller: Use the snarl decomposition to call snarls in a graph
 */
class GraphCaller {
public:
    GraphCaller(SnarlCaller& snarl_caller,
                SnarlManager& snarl_manager,
                ostream& out_stream = cout);

    virtual ~GraphCaller();

    /// Run call_snarl() on every top-level snarl in the manager.
    /// For any that return false, try the children, etc. (when recurse_on_fail true)
    /// Snarls are processed in parallel
    virtual void call_top_level_snarls(bool recurse_on_fail = true);

    /// Call a given snarl, and print the output to out_stream
    virtual bool call_snarl(const Snarl& snarl) = 0;

    /// Write the vcf header (version and contigs and basic info)
    virtual string vcf_header(const PathHandleGraph& graph, const vector<string>& contigs) const;
   
protected:

    /// Our Genotyper
    SnarlCaller& snarl_caller;

    /// Our snarls
    SnarlManager& snarl_manager;

    /// Where all output written
    ostream& out_stream;

};

/**
 * VCFGenotyper : Genotype variants in a given VCF file
 */
class VCFGenotyper : public GraphCaller {
public:
    VCFGenotyper(const PathHandleGraph& graph,
                 SnarlCaller& snarl_caller,
                 SnarlManager& snarl_manager,
                 vcflib::VariantCallFile& variant_file,
                 const string& sample_name,
                 const vector<string>& ref_paths = {},
                 FastaReference* ref_fasta = nullptr,
                 FastaReference* ins_fasta = nullptr,
                 ostream& out_stream = cout);

    virtual ~VCFGenotyper();

    virtual bool call_snarl(const Snarl& snarl);

    virtual string vcf_header(const PathHandleGraph& graph, const vector<string>& contigs) const;

protected:


protected:

    /// the graph
    const PathHandleGraph& graph;

    /// input VCF to genotype, must have been loaded etc elsewhere
    vcflib::VariantCallFile& input_vcf;

    /// output vcf
    mutable vcflib::VariantCallFile output_vcf;

    /// Sample name
    string sample_name;    

    /// traversal finder uses alt paths to map VCF alleles from input_vcf
    /// back to traversals in the snarl
    VCFTraversalFinder traversal_finder;

};


/**
 * LegacyCaller : Preserves (most of) the old vg call logic by using 
 * the RepresentativeTraversalFinder to recursively find traversals
 * through arbitrary sites.   
 */
class LegacyCaller : public GraphCaller {
public:
    LegacyCaller(const PathHandleGraph& graph,
                 SupportBasedSnarlCaller& snarl_caller,
                 SnarlManager& snarl_manager,
                 const string& sample_name,
                 const vector<string>& ref_paths = {});

    virtual ~LegacyCaller();

    virtual bool call_snarl(const Snarl& snarl);

    virtual string vcf_header(const PathHandleGraph& graph, const vector<string>& contigs) const;

protected:

    /// recursively genotype a snarl
    /// todo: can this be pushed to a more generic class? 
    vector<SnarlTraversal> top_down_genotype(const Snarl& snarl, TraversalFinder& trav_finder, int ploidy) const;

    /// print a vcf variant 
    void emit_variant(const Snarl& snarl, const vector<SnarlTraversal>& called_traversals, const string& ref_path_name) const;

    /// check if a site can be handled by the RepresentativeTraversalFinder
    bool is_traversable(const Snarl& snarl);

    /// look up a path index for a site and return its name too
    pair<string, PathIndex*> find_index(const Snarl& snarl, const vector<PathIndex*> path_indexes) const;


protected:

    /// the graph
    const PathHandleGraph& graph;
    /// non-vg inputs are converted into vg as-needed, at least until we get the
    /// traversal finding ported
    bool is_vg;

    /// output vcf
    mutable vcflib::VariantCallFile output_vcf;

    /// Sample name
    string sample_name;    

    /// The old vg call traversal finder.  It is fairly efficient but daunting to maintain.
    /// We keep it around until a better replacement is implemented.  It is *not* compatible
    /// with the Handle Graph API because it relise on PathIndex.  We convert to VG as
    /// needed in order to use it. 
    RepresentativeTraversalFinder* traversal_finder;
    /// Needed by above (only used when working on vg inputs -- generated on the fly otherwise)
    vector<PathIndex*> path_indexes;

    /// keep track of the reference paths
    vector<string> ref_paths;

    /// Tuning

    /// How many nodes should we be willing to look at on our path back to the
    /// primary path? Keep in mind we need to look at all valid paths (and all
    /// combinations thereof) until we find a valid pair.
    int max_search_depth = 1000;
    /// How many search states should we allow on the DFS stack when searching
    /// for traversals?
    int max_search_width = 1000;
    /// What's the maximum number of bubble path combinations we can explore
    /// while finding one with maximum support?
    size_t max_bubble_paths = 100;
    /// What's the minimum integer number of reads that must support a call? We
    /// don't necessarily want to call a SNP as het because we have a single
    // supporting read, even if there are only 10 reads on the site.
    size_t min_total_support_for_call = 1;


};



}

#endif
