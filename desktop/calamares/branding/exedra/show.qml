import QtQuick 2.15
import QtQuick.Controls 2.15

Presentation {
    id: presentation

    Slide {
        centeredText: "Welcome to Exedra.\n\nThe installer copies the live system to disk.\nYour files on the target disk will be erased."
    }

    Slide {
        centeredText: "Exedra uses the venim package manager.\n\nAfter install: venim install <package>."
    }

    function onActivate() { presentation.goToSlide(0); }
    function onLeave() {}
}
