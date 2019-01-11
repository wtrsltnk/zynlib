#ifndef APPSTATE_H
#define APPSTATE_H

extern char const *const AdSynthEditorID;
extern char const *const SubSynthEditorID;
extern char const *const PadSynthEditorID;
extern char const *const InsertionFxEditorID;
extern char const *const SystemFxEditorID;
extern char const *const ChannelFxEditorID;
extern char const *const EffectNames[];
extern int EffectNameCount;
extern const char *NoteNames[];
extern int NoteNameCount;

class AppState
{
public:
    AppState(class Mixer *mixer);
    virtual ~AppState();

    class Mixer *_mixer;
    bool _showLibrary;
    bool _showEditor;
    bool _showInspector;
    bool _showMixer;
    bool _showSystemEffectsEditor;
    bool _showInsertEffectsEditor;
    bool _showChannelEffectsEditor;
    bool _showADNoteEditor;
    bool _showSUBNoteEditor;
    bool _showPADNoteEditor;
    int _showChannelInstrumentSelector;
    int _showChannelTypeChanger;
    int _currentInsertEffect;
    int _currentSystemEffect;
    int _currentChannelEffect;
    int _currentBank;
    int _activeChannel;
    int _activeChannelInstrument;
    int _activePattern;
};

#endif // APPSTATE_H