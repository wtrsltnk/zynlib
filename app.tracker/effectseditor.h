#ifndef EFFECTSEDITOR_H
#define EFFECTSEDITOR_H

#include <zyn.common/globals.h>
#include "applicationsession.h"

class EffectsEditor
{
    ApplicationSession *_session;
public:
    EffectsEditor();

    void SetUp(ApplicationSession *session);
    void Render2d();
};

#endif // EFFECTSEDITOR_H
