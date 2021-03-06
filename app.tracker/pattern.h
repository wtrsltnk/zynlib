#ifndef PATTERN_H
#define PATTERN_H

#include "note.h"

#include <string>
#include <vector>
#include <zyn.common/globals.h>

class Pattern
{
    std::string _name;
    unsigned int _length;
    std::vector<Note> _notes[NUM_MIXER_TRACKS];

public:
    Pattern(std::string const &name, unsigned int length = 64);

    std::string const &Name() const;
    void Rename(std::string const &name);
    unsigned int Length() const;
    void Resize(unsigned int len);
    std::vector<Note> &Notes(unsigned int trackIndex);
};

#endif // PATTERN_H
