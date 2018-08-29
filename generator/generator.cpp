// generator.cpp : Defines the entry point for the console application.
//

#include "expat.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <functional>
#include <exception>
#include <memory>
#include <random>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>
#include <string.h>


using std::cout;
using std::cerr;
using std::exception;
using std::min;
using std::move;
using std::ostringstream;
using std::pair;
using std::remove_pointer;
using std::runtime_error;
using std::uniform_int_distribution;
using std::unique_ptr;
using std::vector;

#ifdef _DEBUG
#define	VERIFY(x)	assert(x)
#else
#define	VERIFY(x)	x
#endif

struct Criterion
{
    int criterion_0;
    int criterion_1;
    int max_instance_number;
    int additional_choices_number;

    int index;
};

struct GeneratorSettings
{
    int min_initial_choice_counter_value;
    int max_initial_choice_counter_value;
    int max_sequence_size;
    int criterion_0_required_value;
    int criterion_1_required_value;

    vector<Criterion> choices;
    vector<Criterion> challenges;

    int numAllChoices;
};

struct SequenceOutput
{
    SequenceOutput() : initialCounter(0) {}

    int initialCounter;
    vector<pair<bool, int>> steps;
};

namespace {

#define SET_PARAMETER(param) if (strcmp(*attribute, #param) == 0) object.param = value; else

void startElement(void *data, const char *element, const char **attribute)
{
    GeneratorSettings* settings = static_cast<GeneratorSettings*>(data);
    if (strcmp(element, "parameters") == 0)
    {
        for (; *attribute != nullptr; attribute += 2)
        {
            int value = 0;
            if (sscanf(*(attribute + 1), "%d", &value) != 1)
                throw runtime_error("Parameters attribute is not an integer.");

            GeneratorSettings& object = *settings;

            SET_PARAMETER(min_initial_choice_counter_value)
            SET_PARAMETER(max_initial_choice_counter_value)
            SET_PARAMETER(max_sequence_size)
            SET_PARAMETER(criterion_0_required_value)
            SET_PARAMETER(criterion_1_required_value)
            throw runtime_error("Attribute is missing.");
        }
    }
    else if (strcmp(element, "choice") == 0)
    {
        Criterion object {}; // zero initialized

        for (; *attribute != nullptr; attribute += 2)
        {
            int value = 0;
            if (sscanf(*(attribute + 1), "%d", &value) != 1)
                throw runtime_error("Choice attribute is not an integer.");

            SET_PARAMETER(criterion_0)
            SET_PARAMETER(criterion_1)
            SET_PARAMETER(max_instance_number)
            SET_PARAMETER(additional_choices_number)
            throw runtime_error("Attribute is missing.");
        }

        object.index = settings->numAllChoices;
        ++settings->numAllChoices;

        auto& collection = object.additional_choices_number ? settings->challenges : settings->choices;
        collection.push_back(object);
    }
}

#undef SET_PARAMETER

void endElement(void * /*data*/, const char * /*el*/)
{
}


GeneratorSettings readGeneratorSettings(const char* fileName)
{
    std::ifstream testFile(fileName);
    std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
        std::istreambuf_iterator<char>());

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, startElement, endElement);

    GeneratorSettings result {}; // zero initialized
    XML_SetUserData(parser, &result);

    unique_ptr<remove_pointer<XML_Parser>::type, void(*)(XML_Parser)> xmlParserGuard(parser, XML_ParserFree);

    if (XML_Parse(parser, fileContents.data(), fileContents.size(), XML_TRUE) == XML_STATUS_ERROR) 
    {
        ostringstream s;
        s << "XML_Parse error: " << XML_ErrorString(XML_GetErrorCode(parser));
        throw runtime_error(s.str());
    }

    return result;
}


bool DoEnumerateChallengeStuff(
    int begin,
    int end,
    int offset,
    const vector<pair<int, int>>& challenges, // int, int => challenge index, additional_choices_number ( > 0)
    vector<pair<int, int>>& indices,
    std::function<bool(const vector<pair<int, int>>&)> callback // pair -> output array index, challenge index
    )
{
    if (offset == static_cast<int>(challenges.size()))
    {
        return callback(indices);
    }

    for (int i = begin; i < end; ++i)
    {
        indices[offset] = { i, challenges[offset].first };
        if (!DoEnumerateChallengeStuff(i + 1, end + challenges[offset].second, offset + 1, challenges, indices, callback))
            return false;
    }

    return true;
}

void EnumerateChallengeStuff(int size, vector<pair<int, int>> challenges, std::function<bool(const vector<pair<int, int>>&)> callback)
{
    std::sort(challenges.begin(), challenges.end());
    vector<pair<int, int>> indices;
    indices.resize(challenges.size());
    while (DoEnumerateChallengeStuff(0, size, 0, challenges, indices, callback) 
        && std::next_permutation(challenges.begin(), challenges.end()))
    {
    }
}


void Report(const GeneratorSettings& generatorSettings, const SequenceOutput& output)
{
    if (output.steps.empty())
    { 
        cout << "No steps can be generated.\n";
    }
    else
    {
        cout << output.initialCounter << '\n';

        bool first = true;
        for (const auto& v : output.steps)
        {
            if (!first)
                cout << ',';
            first = false;
            const Criterion& data = v.first
                ? generatorSettings.challenges[v.second]
                : generatorSettings.choices[v.second];
            cout << data.index;
        }
        cout << '\n';

        int counter = output.initialCounter;
        for (const auto& v : output.steps)
        {
            assert(counter > 0);
            if (v.first)
                counter += generatorSettings.challenges[v.second].additional_choices_number;
            cout << counter << ',';
            --counter;
        }
        assert(counter == 0);
        cout << counter << '\n';

        first = true;
        int criterion_0(0), criterion_1(0);
        for (const auto& v : output.steps)
        {
            if (!first)
                cout << '+';
            first = false;
            cout << '(';
            if (v.first)
                cout << '+' << generatorSettings.challenges[v.second].additional_choices_number << " ch.";
            else
            {
                const Criterion& data = generatorSettings.choices[v.second];
                cout << data.criterion_0 << ',' << data.criterion_1;
                criterion_0 += data.criterion_0;
                criterion_1 += data.criterion_1;
            }
            cout << ')';
        }
        assert(criterion_0 == generatorSettings.criterion_0_required_value);
        assert(criterion_1 == generatorSettings.criterion_1_required_value);
        cout << " = (" << criterion_0 << ',' << criterion_1 << ")\n";

        cout << output.steps.size() << '\n';
    }
}

} // namespace

//////////////////////////////////////////////////////////////////////////////

class Generator
{
public:
    Generator(GeneratorSettings settings) 
        : m_settings(move(settings)) 
        , m_randomEngine(std::random_device{}())
        , m_outcomesCount(0)
    {}

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    SequenceOutput findSequence();
private:

    struct EnumerateCanonicalData
    {
        // first -> index, second -> counter
        vector<pair<int, int>> steps;
        vector<pair<int, int>> challenges;

        int index;
        int challengeIndex;

        int choiceCounter;
        int challengeCounter;
        int challengeAdditionalSteps;

        int criterion_0;
        int criterion_1;

        int initialCounter() const { return choiceCounter - challengeAdditionalSteps; }
        int sequenceSize() const { return choiceCounter + challengeCounter; }
    };

    void EnumerateCanonicalSequencesWithChallenges(EnumerateCanonicalData& data);
    void EnumerateCanonicalSequences(EnumerateCanonicalData& data);
    void HandleCanonicalResult(const EnumerateCanonicalData& data);

    void Trace(const EnumerateCanonicalData& data);

    const GeneratorSettings m_settings;
    std::default_random_engine m_randomEngine;
    unsigned long long m_outcomesCount;

    SequenceOutput m_output;
};

SequenceOutput Generator::findSequence()
{
    EnumerateCanonicalData data {};
    EnumerateCanonicalSequencesWithChallenges(data);
    return m_output;
}

void Generator::EnumerateCanonicalSequencesWithChallenges(EnumerateCanonicalData& data)
{
    if (data.challengeIndex == static_cast<int>(m_settings.challenges.size()))
    {
        EnumerateCanonicalSequences(data);
    }
    else
    {
        const Criterion& challenge = m_settings.challenges[data.challengeIndex];
        const int challengeIndex = data.challengeIndex;
        ++data.challengeIndex;

        EnumerateCanonicalSequencesWithChallenges(data);

        const int threshold = m_settings.max_sequence_size - m_settings.min_initial_choice_counter_value;
        assert(threshold >= 0);

        for (int i = 1; i <= challenge.max_instance_number; ++i)
        {
            if (data.challengeCounter + data.challengeAdditionalSteps + i > threshold
                || data.criterion_0 + challenge.criterion_0 * i > m_settings.criterion_0_required_value
                || data.criterion_1 + challenge.criterion_1 * i > m_settings.criterion_1_required_value)
                break;

            data.challengeAdditionalSteps += i * (challenge.additional_choices_number - 1);
            data.challengeCounter += i;
            data.criterion_0 += challenge.criterion_0 * i;
            data.criterion_1 += challenge.criterion_1 * i;
            data.challenges.push_back({ challengeIndex, i });

            EnumerateCanonicalSequences(data);

            data.challenges.pop_back();
            data.criterion_0 -= challenge.criterion_0 * i;
            data.criterion_1 -= challenge.criterion_1 * i;
            data.challengeCounter -= i;
            data.challengeAdditionalSteps -= i * (challenge.additional_choices_number - 1);
        }

        --data.challengeIndex;
    }
}

void Generator::EnumerateCanonicalSequences(EnumerateCanonicalData& data)
{
    const int threshold = min(m_settings.max_sequence_size - data.challengeCounter,
        m_settings.max_initial_choice_counter_value + data.challengeAdditionalSteps);

    if (data.index == static_cast<int>(m_settings.choices.size()))
    {
        if (data.choiceCounter >= m_settings.min_initial_choice_counter_value + data.challengeAdditionalSteps
            && data.choiceCounter <= threshold
            && data.criterion_0 == m_settings.criterion_0_required_value
            && data.criterion_1 == m_settings.criterion_1_required_value)
        {
            HandleCanonicalResult(data);
        }
    }
    else
    {
        const Criterion& choice = m_settings.choices[data.index];
        const int index = data.index;
        ++data.index;

        EnumerateCanonicalSequences(data);

        for (int i = 1; i <= choice.max_instance_number; ++i)
        {
            if (data.choiceCounter + i > threshold
                    || data.criterion_0 + choice.criterion_0 * i > m_settings.criterion_0_required_value
                    || data.criterion_1 + choice.criterion_1 * i > m_settings.criterion_1_required_value)
                break;

            data.choiceCounter += i;
            data.criterion_0 += choice.criterion_0 * i;
            data.criterion_1 += choice.criterion_1 * i;
            data.steps.push_back({ index, i });

            EnumerateCanonicalSequences(data);

            data.steps.pop_back();
            data.criterion_0 -= choice.criterion_0 * i;
            data.criterion_1 -= choice.criterion_1 * i;
            data.choiceCounter -= i;
        }

        --data.index;
    }
}

void Generator::HandleCanonicalResult(const EnumerateCanonicalData& data)
{
    assert(data.initialCounter() >= m_settings.min_initial_choice_counter_value);
    assert(data.initialCounter() <= m_settings.max_initial_choice_counter_value);
    assert(data.sequenceSize() <= m_settings.max_sequence_size);

    // Count result variations
    vector<pair<int, int>> challenges;
    for (const auto& v : data.challenges)
        challenges.insert(challenges.end(), v.second, { v.first, m_settings.challenges[v.first].additional_choices_number });

    unsigned long long challengesCount = 0;
    EnumerateChallengeStuff(data.initialCounter(), challenges,
                            [&challengesCount](const vector<pair<int, int>>&) 
                            {
                              ++challengesCount;
                              return true;
                            });

    unsigned long long choicesCount = 0;
    {
        vector<int> choices;
        for (const auto& v : data.steps)
            choices.insert(choices.end(), v.second, v.first);

        sort(choices.begin(), choices.end());
        do
        {
            ++choicesCount;
        } while (std::next_permutation(choices.begin(), choices.end()));
    }

    const unsigned long long outcomesCount = challengesCount * choicesCount;
    m_outcomesCount += outcomesCount;

    if (uniform_int_distribution<unsigned long long>(0, m_outcomesCount - 1)(m_randomEngine) >= outcomesCount)
        return;

    m_output.initialCounter = data.initialCounter();

    // generate random sequence
    vector<pair<int, int>> randomChallenges; // pair -> output array index, challenge index
    {
        const unsigned long long randomChallenge = uniform_int_distribution<unsigned long long>(0, challengesCount - 1)(m_randomEngine);
        unsigned long long challengeIndex = 0;
        EnumerateChallengeStuff(data.initialCounter(), challenges,
            [&challengeIndex, randomChallenge, &randomChallenges](const vector<pair<int, int>>& data)
        {
            if (challengeIndex++ == randomChallenge)
            {
                randomChallenges = data;
                return false;
            }
            return true;
        });
        assert(challengeIndex == randomChallenge + 1);
        std::sort(randomChallenges.begin(), randomChallenges.end());
    }

    m_output.steps.clear();
    for (const auto& v : data.steps)
        m_output.steps.insert(m_output.steps.end(), v.second, { false, v.first });

    sort(m_output.steps.begin(), m_output.steps.end());
    const unsigned long long randomSequence = uniform_int_distribution<unsigned long long>(0, choicesCount - 1)(m_randomEngine);
    for (unsigned long long i = 0; i < randomSequence; ++i)
    {
        VERIFY(std::next_permutation(m_output.steps.begin(), m_output.steps.end()));
    }

    for (const auto& v : randomChallenges)
    {
        m_output.steps.insert(m_output.steps.begin() + v.first, { true, v.second });
    }
}

void Generator::Trace(const EnumerateCanonicalData& data)
{
    cout << data.initialCounter();
    for (const auto& challenge : data.challenges)
    {
        const Criterion& criterion = m_settings.challenges[challenge.first];
        for (int i = 0; i < challenge.second; ++i)
            cout << ' ' << criterion.index << "(+" << criterion.additional_choices_number << " )";
    }
    for (const auto& step : data.steps)
    {
        const Criterion& criterion = m_settings.choices[step.first];
        for (int i = 0; i < step.second; ++i)
            cout << ' ' << criterion.index << '(' << criterion.criterion_0 << ',' << criterion.criterion_1 << ')';
    }
    cout << ' ' << data.sequenceSize() << std::endl;
}


//////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: generator input_file\n";
        return 1;
    }

    try
    {
        GeneratorSettings generatorSettings(readGeneratorSettings(argv[1]));
        for (int i = 0; i < 1000; ++i) {
            Generator generator(generatorSettings);
            SequenceOutput output = generator.findSequence();
            Report(generatorSettings, output);
        }
    }
    catch (const exception& e)
    {
        cerr << e.what();
        return 1;
    }

    return 0;
}
