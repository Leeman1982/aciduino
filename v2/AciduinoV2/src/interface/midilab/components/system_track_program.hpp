struct TrackProgramSelector : PageComponent {

    void view() {
      // GM programs are 1-128 for humans; for the 808 drum track this selects
      // the drum kit (e.g. program 26 = TR-808 kit on many GM modules)
      genericOptionView("instr", String(aciduino.getTrackOutputParam(TRACK_PROGRAM)+1), line, col, selected);
    }

    void change(int16_t data) {
      data = parseData(data, 0, 127, aciduino.getTrackOutputParam(TRACK_PROGRAM));
      // setTrackOutputParam(TRACK_PROGRAM, ...) stores the value AND sends the
      // Program Change to the track's port/channel so you hear it while scrolling
      aciduino.setTrackOutputParam(TRACK_PROGRAM, data);
    }

} trackProgramSelectorComponent;
