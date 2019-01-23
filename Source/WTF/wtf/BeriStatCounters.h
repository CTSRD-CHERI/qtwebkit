#pragma once

#if __has_include(<statcounters.h>)
#include <statcounters.h>
#include <string>

#define statcountersDeclareBank(bank_name) statcounters_bank_t bank_name
static inline void statcountersStartPhase(statcounters_bank_t *stat_start, const std::string& phase) {
    fprintf(stderr, " -- Starting phase %s\n", phase.c_str());
    memset(stat_start, 0, sizeof(*stat_start));
    statcounters_sample(stat_start); // reset start
}
static inline void statcountersEndPhase(const statcounters_bank_t *stat_start, const std::string& phase) {
    statcounters_bank_t stat_end;
    statcounters_bank_t stat_diff;
    statcounters_sample(&stat_end);
    statcounters_diff(&stat_diff, &stat_end, stat_start);
    // TODO: actually dump as CSV
    // statcounters_dump_with_phase(&stat_diff, phase.c_str());
    fprintf(stderr, " -- Finished phase %s after %jd instructions\n", phase.c_str(), (intmax_t)stat_diff.inst);
}

class BeriStatCountersScope {
    statcounters_bank_t start;
    std::string name;
public:
    BeriStatCountersScope(const std::string&& name) : name(std::move(name)) {
        statcountersStartPhase(&start, this->name);
    }
    ~BeriStatCountersScope() {
        statcountersEndPhase(&start, this->name);
    }
};

#define statcountersMeasureScope(name) BeriStatCountersScope __CONCAT(_statcounter_scope, __LINE__)(name)

#else
#define statcountersMeasureScope(name) ((void)0)
#define statcountersDeclareBank(name) ((void)0)
#define statcountersStartPhase(start, phase) ((void)0)
#define statcountersEndPhase(start, phase) ((void)0)
#endif
#define statcountersDeclareAndStartPhase(start, phase) statcountersDeclareBank(start); statcountersStartPhase(&start, phase)
