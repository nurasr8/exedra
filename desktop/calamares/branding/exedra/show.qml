import QtQuick 2.0;
import calamares.slideshow 1.0;

Presentation
{
    id: presentation

    function nextSlide() {
        presentation.goToNextSlide();
    }

    Timer {
        id: advanceTimer
        interval: 8000
        running: true
        repeat: true
        onTriggered: nextSlide()
    }

    Slide {
        centeredText: "Welcome to Exedra.\n\nThe installer copies the live system to disk.\nAll data on the target disk will be erased."
    }

    Slide {
        centeredText: "Exedra uses the venim package manager.\n\nAfter install: venim install <package>."
    }

    function onActivate() { presentation.goToSlide(0); }
    function onLeave() {}
}
