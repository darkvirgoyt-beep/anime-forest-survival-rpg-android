#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <iterator>
#include <string>
#include <vector>
#include <utility>

#include "rpg/cloud_state.h"

namespace {

constexpr int kWarmupIterations = 10000;
constexpr int kSamples = 200;
constexpr int kIterationsPerSample = 1000;
volatile std::uint64_t gBenchmarkSink = 0;

struct BenchmarkCase {
    const char* name;
    const char* payload;
    int sourceSchema;
};

struct Summary {
    double meanMicros;
    double p50Micros;
    double p95Micros;
    double p99Micros;
    double minMicros;
    double maxMicros;
};

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "cloud_state_benchmark: FAIL: " << message << '\n';
        std::exit(1);
    }
}

Summary summarize(std::vector<double> samples) {
    std::sort(samples.begin(), samples.end());
    const double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
    const auto percentile = [&samples](std::size_t numerator, std::size_t denominator) {
        const std::size_t index = std::min(samples.size() - 1, (samples.size() * numerator + denominator - 1) / denominator - 1);
        return samples[index];
    };
    return {
        mean,
        percentile(50, 100),
        percentile(95, 100),
        percentile(99, 100),
        samples.front(),
        samples.back()
    };
}

Summary benchmark(const BenchmarkCase& testCase) {
    for (int i = 0; i < kWarmupIterations; ++i) {
        forest::rpg::CloudState state{};
        require(forest::rpg::parseCloudState(testCase.payload, state), "warmup parse failed");
        gBenchmarkSink += static_cast<std::uint64_t>(state.schemaVersion + state.sourceSchemaVersion + state.discoveredSectors);
    }

    std::vector<double> samples;
    samples.reserve(kSamples);
    for (int sample = 0; sample < kSamples; ++sample) {
        const auto start = std::chrono::steady_clock::now();
        std::uint64_t checksum = 0;
        for (int iteration = 0; iteration < kIterationsPerSample; ++iteration) {
            forest::rpg::CloudState state{};
            require(forest::rpg::parseCloudState(testCase.payload, state), "benchmark parse failed");
            checksum += static_cast<std::uint64_t>(state.schemaVersion + state.sourceSchemaVersion + state.discoveredSectors);
        }
        const auto elapsed = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
        gBenchmarkSink += checksum;
        samples.push_back(elapsed / kIterationsPerSample);
    }
    return summarize(std::move(samples));
}

void writeJson(const std::string& path, const std::vector<std::pair<BenchmarkCase, Summary>>& results) {
    std::ofstream output(path);
    require(output.good(), "could not open JSON output");
    output << "{\n  \"profile\": \"single-core constrained proxy; host clock, not a physical Android measurement\",\n"
           << "  \"warmupIterations\": " << kWarmupIterations << ",\n"
           << "  \"samples\": " << kSamples << ",\n"
           << "  \"iterationsPerSample\": " << kIterationsPerSample << ",\n"
           << "  \"results\": [\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& [testCase, summary] = results[i];
        output << std::fixed << std::setprecision(4)
               << "    {\"schema\": \"" << testCase.name << "\", \"sourceSchema\": " << testCase.sourceSchema
               << ", \"meanMicros\": " << summary.meanMicros
               << ", \"p50Micros\": " << summary.p50Micros
               << ", \"p95Micros\": " << summary.p95Micros
               << ", \"p99Micros\": " << summary.p99Micros
               << ", \"minMicros\": " << summary.minMicros
               << ", \"maxMicros\": " << summary.maxMicros << "}";
        if (i + 1 != results.size()) output << ',';
        output << '\n';
    }
    output << "  ],\n  \"sink\": " << gBenchmarkSink << "\n}\n";
}

} // namespace

int main(int argc, char** argv) {
    constexpr BenchmarkCase cases[] = {
        {"schema-5-full", "{\"schemaVersion\":5,\"playerX\":0.340000,\"playerY\":-0.220000,\"health\":0.680000,\"stamina\":0.420000,\"hunger\":0.730000,\"wood\":19,\"fiber\":7,\"stone\":12,\"experience\":840,\"level\":8,\"experienceToNext\":810,\"totalExperience\":5040,\"day\":4,\"worldTime\":512.500000,\"gatheringActions\":3,\"questStage\":3,\"emberKitCrafted\":1,\"wardenDefeated\":1,\"emberlingTrust\":3,\"emberlingBonded\":1,\"emberlingStay\":1,\"discoveredSectors\":15,\"capturedMobIndex\":3,\"capturedCompanionStay\":1,\"campBuilt\":1,\"campX\":-0.640000,\"campY\":0.520000,\"campZ\":0.000000,\"campYaw\":0.000000,\"campScale\":1.000000,\"companionRevision\":4,\"campRevision\":9}", 5},
        {"schema-4-legacy", "{\"schemaVersion\":4,\"playerX\":0.2,\"playerY\":-0.2,\"health\":0.9,\"stamina\":0.8,\"hunger\":0.7,\"wood\":8,\"fiber\":9,\"stone\":10,\"experience\":230,\"level\":3,\"experienceToNext\":1800,\"totalExperience\":3230,\"day\":5,\"worldTime\":25,\"gatheringActions\":3,\"questStage\":2,\"emberKitCrafted\":1,\"wardenDefeated\":0,\"emberlingTrust\":3,\"emberlingBonded\":1,\"emberlingStay\":1}", 4},
        {"schema-3-legacy", "{\"schemaVersion\":3,\"playerX\":0.1,\"playerY\":-0.1,\"health\":0.8,\"stamina\":0.7,\"hunger\":0.6,\"wood\":5,\"fiber\":6,\"stone\":7,\"experience\":120,\"level\":2,\"experienceToNext\":1500,\"totalExperience\":2120,\"day\":3,\"worldTime\":15,\"gatheringActions\":2,\"questStage\":1,\"emberKitCrafted\":1,\"wardenDefeated\":0}", 3},
        {"schema-2-legacy", "{\"schemaVersion\":2,\"playerX\":0,\"playerY\":0,\"health\":0.5,\"stamina\":0.5,\"hunger\":0.5,\"wood\":1,\"fiber\":2,\"stone\":3,\"experience\":84,\"day\":2,\"worldTime\":5,\"gatheringActions\":1,\"questStage\":1,\"emberKitCrafted\":0,\"wardenDefeated\":0}", 2},
        {"schema-1-legacy", "{\"schemaVersion\":1,\"playerX\":0,\"playerY\":0,\"health\":0.9,\"stamina\":0.8,\"hunger\":0.7,\"wood\":4,\"fiber\":3,\"stone\":2,\"experience\":42,\"day\":1,\"worldTime\":2}", 1}
    };

    std::vector<std::pair<BenchmarkCase, Summary>> results;
    results.reserve(std::size(cases));
    for (const BenchmarkCase& testCase : cases) {
        forest::rpg::CloudState migrated{};
        require(forest::rpg::parseCloudState(testCase.payload, migrated), "correctness parse failed");
        require(migrated.schemaVersion == 5, "accepted save did not canonicalize to schema 5");
        require(migrated.sourceSchemaVersion == testCase.sourceSchema, "source schema metadata was not retained");
        const std::string serialized = forest::rpg::serializeCloudState(migrated);
        require(serialized.find("\"schemaVersion\":5") != std::string::npos, "migration serialization is not schema 5");
        results.emplace_back(testCase, benchmark(testCase));
    }

    std::cout << "profile=single-core-constrained-proxy\n"
              << "warmup=" << kWarmupIterations << " samples=" << kSamples
              << " iterations_per_sample=" << kIterationsPerSample << "\n"
              << "schema,source_schema,mean_us,p50_us,p95_us,p99_us,min_us,max_us,throughput_ops_s\n";
    for (const auto& [testCase, summary] : results) {
        std::cout << std::fixed << std::setprecision(4)
                  << testCase.name << ',' << testCase.sourceSchema << ',' << summary.meanMicros << ','
                  << summary.p50Micros << ',' << summary.p95Micros << ',' << summary.p99Micros << ','
                  << summary.minMicros << ',' << summary.maxMicros << ',' << (1000000.0 / summary.meanMicros) << '\n';
    }
    if (argc > 1) writeJson(argv[1], results);
    std::cerr << "benchmark_sink=" << gBenchmarkSink << '\n';
    return 0;
}
