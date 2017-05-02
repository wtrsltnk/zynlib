/*
  ZynAddSubFX - a software synthesizer

  main.cpp  -  Main file of the synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2012-2014 Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <FL/Fl.H>

#include "UI/common.H"

#include <iostream>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <signal.h>
#include <time.h>

#include <unistd.h>
#include <pthread.h>

#include <getopt.h>

#include "../zyn.synth/FFTwrapper.h"
#include "../zyn.mixer/Mixer.h"
#include "../zyn.mixer/Instrument.h"
#include "../zyn.common/Util.h"

// Sequencer
#include "../zyn.seq/Sequencer.h"

//Nio System
#include "../zyn.nio/Nio.h"

#ifdef ENABLE_FLTKGUI
#include "MasterUI.h"

static MasterUI *ui;

#endif //ENABLE_FLTKGUI

using namespace std;

static Mixer* mixer;
static Sequencer* seq;
//SYNTH_T* synth;
static int swaplr = 0; //1 for left-right swapping

static int Pexitprogram = 0;     //if the UI set this to 1, the program will exit

//cleanup on signaled exit
void sigterm_exit(int /*sig*/)
{
    Pexitprogram = 1;
}


#ifdef ENABLE_FLTKGUI
void set_module_parameters ( Fl_Widget *o )
{
    o->box( FL_PLASTIC_THIN_UP_BOX );
    o->color( FL_BLACK );
    o->labeltype( FL_NORMAL_LABEL );
}
#endif

/*
 * Program exit
 */
int exitprogram()
{
    //ensure that everything has stopped with the mutex wait
    mixer->Lock();
    mixer->Unlock();

    seq->Quit();

    Nio::stop();

#ifdef ENABLE_FLTKGUI
    delete ui;
#endif // ENABLE_FLTKGUI

    delete mixer->_synth;
    delete mixer;
    FFT_cleanup();

    return 0;
}

int main(int argc, char *argv[])
{
    auto synth = new SystemSettings;
    config.init();
    int noui = 0;
    cerr
            << "\nZynAddSubFX - Copyright (c) 2002-2011 Nasca Octavian Paul and others"
            << endl;
    cerr
            << "                Copyright (c) 2009-2014 Mark McCurry [active maintainer]"
            << endl;
    cerr << "Compiled: " << __DATE__ << " " << __TIME__ << endl;
    cerr << "This program is free software (GNU GPL v.2 or later) and \n";
    cerr << "it comes with ABSOLUTELY NO WARRANTY.\n" << endl;
    if(argc == 1)
        cerr << "Try 'zynaddsubfx --help' for command-line options." << endl;

    /* Get the settings from the Config*/
    synth->samplerate = config.cfg.SampleRate;
    synth->buffersize = config.cfg.SoundBufferSize;
    synth->oscilsize  = config.cfg.OscilSize;
    swaplr = config.cfg.SwapStereo;

    Nio::preferedSampleRate(synth->samplerate);

    synth->alias(); //build aliases

    sprng(time(NULL));

    /* Parse command-line options */
    struct option opts[] = {
        { "load", 2, NULL, 'l' },
        { "load-instrument", 2, NULL, 'L' },
        { "sample-rate", 2, NULL, 'r' },
        { "buffer-size", 2, NULL, 'b' },
        { "oscil-size", 2, NULL, 'o' },
        { "dump", 2, NULL, 'D' },
        { "swap", 2, NULL, 'S' },
        { "no-gui", 2, NULL, 'U' },
        { "dummy", 2, NULL, 'Y' },
        { "help", 2, NULL, 'h' },
        { "version", 2, NULL, 'v' },
        { "named", 1, NULL, 'N' },
        { "auto-connect", 0, NULL, 'a' },
        { "output", 1, NULL, 'O' },
        { "input", 1, NULL, 'I' },
        { "exec-after-init", 1, NULL, 'e' },
        { 0, 0, 0, 0 }
    };
    opterr = 0;
    int option_index = 0, opt, exitwithhelp = 0, exitwithversion = 0;

    string loadfile, loadinstrument, execAfterInit;

    while(1) {
        int tmp = 0;

        /**\todo check this process for a small memory leak*/
        opt = getopt_long(argc,
                          argv,
                          "l:L:r:b:o:I:O:N:e:hvaSDUY",
                          opts,
                          &option_index);
        char *optarguments = optarg;

#define GETOP(x) if(optarguments) \
    x = optarguments
#define GETOPNUM(x) if(optarguments) \
    x = atoi(optarguments)


        if(opt == -1)
            break;

        switch(opt) {
        case 'h':
            exitwithhelp = 1;
            break;
        case 'v':
            exitwithversion = 1;
            break;
        case 'Y': /* this command a dummy command (has NO effect)
                        and is used because I need for NSIS installer
                        (NSIS sometimes forces a command line for a
                        program, even if I don't need that; eg. when
                        I want to add a icon to a shortcut.
                     */
            break;
        case 'U':
            noui = 1;
            break;
        case 'l':
            GETOP(loadfile);
            break;
        case 'L':
            GETOP(loadinstrument);
            break;
        case 'r':
            GETOPNUM(synth->samplerate);
            if(synth->samplerate < 4000) {
                cerr << "ERROR:Incorrect sample rate: " << optarguments
                     << endl;
                exit(1);
            }
            break;
        case 'b':
            GETOPNUM(synth->buffersize);
            if(synth->buffersize < 2) {
                cerr << "ERROR:Incorrect buffer size: " << optarguments
                     << endl;
                exit(1);
            }
            break;
        case 'o':
            if(optarguments)
                synth->oscilsize = tmp = atoi(optarguments);
            if(synth->oscilsize < MAX_AD_HARMONICS * 2)
                synth->oscilsize = MAX_AD_HARMONICS * 2;
            synth->oscilsize =
                    (int) powf(2,
                               ceil(logf(synth->oscilsize - 1.0f) / logf(2.0f)));
            if(tmp != synth->oscilsize)
                cerr
                        <<
                           "synth->oscilsize is wrong (must be 2^n) or too small. Adjusting to "
                        << synth->oscilsize << "." << endl;
            break;
        case 'S':
            swaplr = 1;
            break;
        case 'I':
            if(optarguments)
                Nio::setDefaultSource(optarguments);
            break;
        case 'O':
            if(optarguments)
                Nio::setDefaultSink(optarguments);
            break;
        case 'e':
            GETOP(execAfterInit);
            break;
        case '?':
            cerr << "ERROR:Bad option or parameter.\n" << endl;
            exitwithhelp = 1;
            break;
        }
    }

    if(exitwithversion) {
        cout << "Version: " << VERSION << endl;
        return 0;
    }
    if(exitwithhelp != 0) {
        cout << "Usage: zynaddsubfx [OPTION]\n\n"
             << "  -h , --help \t\t\t\t Display command-line help and exit\n"
             << "  -v , --version \t\t\t Display version and exit\n"
             << "  -l file, --load=FILE\t\t\t Loads a .xmz file\n"
             << "  -L file, --load-instrument=FILE\t Loads a .xiz file\n"
             << "  -r SR, --sample-rate=SR\t\t Set the sample rate SR\n"
             <<
                "  -b BS, --buffer-size=SR\t\t Set the buffer size (granularity)\n"
             << "  -o OS, --oscil-size=OS\t\t Set the ADsynth oscil. size\n"
             << "  -S , --swap\t\t\t\t Swap Left <--> Right\n"
             << "  -D , --dump\t\t\t\t Dumps midi note ON/OFF commands\n"
             <<
                "  -U , --no-gui\t\t\t\t Run ZynAddSubFX without user interface\n"
             << "  -O , --output\t\t\t\t Set Output Engine\n"
             << "  -I , --input\t\t\t\t Set Input Engine\n"
             << "  -e , --exec-after-init\t\t Run post-initialization script\n"
             << endl;

        return 0;
    }

    synth->alias();

    cerr.precision(1);
    cerr << std::fixed;
    cerr << "\nSample Rate = \t\t" << synth->samplerate << endl;
    cerr << "Sound Buffer Size = \t" << synth->buffersize << " samples" << endl;
    cerr << "Internal latency = \t\t" << synth->buffersize_f * 1000.0f / synth->samplerate_f << " ms" << endl;
    cerr << "ADsynth Oscil.Size = \t" << synth->oscilsize << " samples" << endl;

    signal(SIGINT, sigterm_exit);
    signal(SIGTERM, sigterm_exit);
    mixer = new Mixer(synth);
    mixer->swaplr = swaplr;

    if(!loadfile.empty()) {
        int tmp = mixer->loadXML(loadfile.c_str());
        if(tmp < 0) {
            cerr << "ERROR: Could not load master file " << loadfile
                 << "." << endl;
            exit(1);
        }
        else {
            mixer->applyparameters();
            cout << "Master file loaded." << endl;
        }
    }

    if(!loadinstrument.empty()) {
        int tmp = mixer->part[0]->loadXMLinstrument(
                    loadinstrument.c_str());
        if(tmp < 0) {
            cerr << "ERROR: Could not load instrument file "
                 << loadinstrument << '.' << endl;
            exit(1);
        }
        else {
            mixer->part[0]->applyparameters();
            cout << "Instrument file loaded." << endl;
        }
    }

    //Run the Nio system
    bool ioGood = Nio::start(mixer);

    // Run a system command after starting zynaddsubfx
    if(!execAfterInit.empty()) {
        cout << "Executing user supplied command: " << execAfterInit << endl;
        if(system(execAfterInit.c_str()) == -1)
            cerr << "Command Failed..." << endl;
    }

    seq = new Sequencer(mixer);

#ifdef ENABLE_FLTKGUI

    ui = new MasterUI(mixer, seq, &Pexitprogram);
    
    if ( !noui)
    {
        ui->showUI();

        if(!ioGood)
            fl_alert("Default IO did not initialize.\nDefaulting to NULL backend.");
    }

#endif

    while(Pexitprogram == 0) {
#ifdef ENABLE_FLTKGUI
        Fl::wait(0.02f);
#else // ENABLE_FLTKGUI
        usleep(100000);
#endif // ENABLE_FLTKGUI
    }

    return exitprogram();
}
