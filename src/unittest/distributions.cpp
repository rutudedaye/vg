/**
 * unittest/distributions.cpp: test cases for distributions.hpp
 */

#include "catch.hpp"
#include "distributions.hpp"

namespace vg {
namespace unittest {

TEST_CASE( "Factorials are computed", "[distributions][factorial]" ) {
    REQUIRE(factorial_ln(0) == Approx(log(1)).epsilon(1E-10));
    REQUIRE(factorial_ln(1) == Approx(log(1)).epsilon(1E-10));
    REQUIRE(factorial_ln(2) == Approx(log(2)).epsilon(1E-10));
    REQUIRE(factorial_ln(3) == Approx(log(6)).epsilon(1E-10));
    REQUIRE(factorial_ln(10) == Approx(log(3628800)).epsilon(1E-10));
}

TEST_CASE( "Ambiguous multinomial works", "[distributions][multinomial]" ) {

    SECTION("An empty case can be handled") {
    
        // Define some category probabilities
        vector<double> probs{0.5, 0.25, 0.25};
        
        // Try classes but no reads
        unordered_map<vector<bool>, int> obs{{{true, false, false}, 0}, {{false, true, false}, 0}};
        
        // No reads are always matching
        REQUIRE(logprob_to_prob(multinomial_censored_sampling_prob_ln(probs, obs)) == Approx(1.0).epsilon(1E-10));
        
        obs.clear();
        
        // No reads are still always matching
        REQUIRE(logprob_to_prob(multinomial_censored_sampling_prob_ln(probs, obs)) == Approx(1.0).epsilon(1E-10));
    
    }
    
    SECTION("An empty case with no categories can be handled") {
        vector<double> probs;
        // Define 0 observations of things that match all of no categories.
        unordered_map<vector<bool>, int> obs{{vector<bool>{}, 0}};
        
        // No reads are always matching
        REQUIRE(logprob_to_prob(multinomial_censored_sampling_prob_ln(probs, obs)) == Approx(1.0).epsilon(1E-10));
        
        obs.clear();
        
        // No reads are still always matching
        REQUIRE(logprob_to_prob(multinomial_censored_sampling_prob_ln(probs, obs)) == Approx(1.0).epsilon(1E-10));
    }

    SECTION("An unambiguous case can be handled") {

        // Define some category probabilities
        vector<double> probs{0.5, 0.25, 0.25};
        
        // And an ambiguous observation: 1 in category 1, 2 split between 2 and 3.
        unordered_map<vector<bool>, int> obs{{{true, false, false}, 15}, {{false, true, false}, 36}};
        
        // Define the only real possible case matching the observations
        vector<int> case1{15, 36, 0};
        
        // Sum up case probabilities
        auto case_total = multinomial_sampling_prob_ln(probs, case1);

        // Make sure we get the same answer for the probability of matching those constraints.
        REQUIRE(multinomial_censored_sampling_prob_ln(probs, obs) == Approx(case_total).epsilon(1E-10));
    }

    SECTION("A simple 3-category ambiguous case can be handled") {

        // Define some category probabilities
        vector<double> probs{0.5, 0.25, 0.25};
        
        // And an ambiguous observation: 1 in category 1, 2 split between 2 and 3.
        unordered_map<vector<bool>, int> obs{{{true, false, false}, 1}, {{false, true, true}, 2}};
        
        // Define the 3 real possible cases matching the observations
        vector<int> case1{1, 2, 0};
        vector<int> case2{1, 1, 1};
        vector<int> case3{1, 0, 2};
        
        // Sum up their probabilities
        auto case_total = logprob_add(logprob_add(multinomial_sampling_prob_ln(probs, case1),
            multinomial_sampling_prob_ln(probs, case2)), multinomial_sampling_prob_ln(probs, case3));

        // Make sure we get the same answer for the probability of matching those constraints.
        REQUIRE(multinomial_censored_sampling_prob_ln(probs, obs) == Approx(case_total).epsilon(1E-10));
        
    }
    
    SECTION("A case with multiple kinds of highly ambiguous reads can be handled") {
        
        vector<double> probs{0.5, 0.20, 0.25, 0.05};
        
        // And an ambiguous observation: 1 read in each of a few classes
        unordered_map<vector<bool>, int> obs{
            {{true, true, false, false}, 1},
            {{false, true, true, false}, 1},
            {{false, true, true, true}, 1}
        };
        
        // Define the real possible cases matching the observations
        vector<vector<int>> cases;
        cases.emplace_back(vector<int>{1, 2, 0, 0});
        cases.emplace_back(vector<int>{1, 1, 1, 0});
        cases.emplace_back(vector<int>{1, 1, 0, 1});
        cases.emplace_back(vector<int>{1, 1, 1, 0});
        cases.emplace_back(vector<int>{1, 0, 2, 0});
        cases.emplace_back(vector<int>{1, 0, 1, 1});
        cases.emplace_back(vector<int>{0, 3, 0, 0});
        cases.emplace_back(vector<int>{0, 2, 1, 0});
        cases.emplace_back(vector<int>{0, 2, 0, 1});
        cases.emplace_back(vector<int>{0, 2, 1, 0});
        cases.emplace_back(vector<int>{0, 1, 2, 0});
        cases.emplace_back(vector<int>{0, 1, 1, 1});
        
        vector<double> case_logprobs;
        for (auto this_case : cases) {
            // Compute probs for all these real cases
            case_logprobs.push_back(multinomial_sampling_prob_ln(probs, this_case));
        }
        
        // Make sure we get the same answer for the probability of matching those constraints.
        REQUIRE(multinomial_censored_sampling_prob_ln(probs, obs) == Approx(logprob_sum(case_logprobs)).epsilon(1E-10));
    }
    
    SECTION("A class with a lot of reads can be handled") {
        vector<double> probs{0.5, 0.20, 0.25, 0.05};
        
        // Just make everything ambiguous.
        unordered_map<vector<bool>, int> obs{
            {{true, true, true, true}, 100}
        };
        
        // The likelihood overall should be 1, because all assignments of 100 reads match the constraint.
        // But we have to give a bit more tolerance because we're summing up really tiny numbers. Also, compare in prob space.
        REQUIRE(logprob_to_prob(multinomial_censored_sampling_prob_ln(probs, obs)) == Approx(1.0).epsilon(1E-9));
    
    }

}

}
}
