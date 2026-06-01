#ifndef STYLES_H
#define STYLES_H

#include <QString>

namespace AppStyles {

inline QString applicationStyleSheet()
{
    return QStringLiteral(R"(
        QWidget#appRoot {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #FFF9E6,
                stop:0.45 #E8F8FF,
                stop:1 #FFE8F3
            );
        }
        QMainWindow {
            background: transparent;
        }
        QWidget {
            color: #2D3436;
            font-family: "Comic Sans MS", "Segoe UI", sans-serif;
            font-size: 16pt;
        }
        QFrame#cardPanel {
            background-color: #FFFFFF;
            border: 4px solid #FFD93D;
            border-radius: 28px;
        }
        QLabel#emojiLabel {
            font-size: 52pt;
            background: transparent;
            border: none;
        }
        QLabel#titleLabel {
            font-size: 28pt;
            font-weight: bold;
            color: #FF6B9D;
            background: transparent;
        }
        QLabel#subtitleLabel {
            font-size: 15pt;
            color: #6C5CE7;
            background: transparent;
        }
        QLabel#formLabel {
            font-size: 15pt;
            font-weight: bold;
            color: #00B894;
            padding: 0;
            margin: 0;
        }
        QLabel#statusLabel {
            font-size: 14pt;
            color: #636E72;
            background: #F8F9FA;
            border-radius: 12px;
            padding: 10px 14px;
        }
        QLabel#sectionLabel {
            font-size: 17pt;
            font-weight: bold;
            color: #6C5CE7;
            padding: 4px 0;
        }
        QLineEdit {
            background-color: #FFFBF0;
            border: 3px solid #74B9FF;
            border-radius: 14px;
            padding: 12px 16px;
            font-size: 16pt;
            min-height: 30px;
        }
        QLineEdit:focus {
            border-color: #FF6B9D;
            background-color: #FFFFFF;
        }
        QTextEdit {
            background-color: #FFFBF0;
            border: 3px solid #55EFC4;
            border-radius: 18px;
            padding: 14px;
            font-size: 15pt;
            line-height: 1.35;
        }
        QPushButton {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #FF8A8A, stop:1 #FF6B6B
            );
            color: white;
            border: 3px solid #FFFFFF;
            border-radius: 20px;
            padding: 10px 20px;
            font-size: 16pt;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #FF6B9D, stop:1 #E84393
            );
        }
        QPushButton:pressed {
            padding-top: 16px;
            padding-bottom: 12px;
        }
        QPushButton#secondaryButton {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #81ECEC, stop:1 #48DBFB
            );
            color: #2D3436;
        }
        QPushButton#secondaryButton:hover {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #48DBFB, stop:1 #0984E3
            );
            color: white;
        }
        QPushButton#successButton {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #7BED9F, stop:1 #2ECC71
            );
            color: #2D3436;
        }
        QPushButton#successButton:hover {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #55EFC4, stop:1 #00B894
            );
            color: white;
        }
        QPushButton#accentButton {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #DDA0FF, stop:1 #A29BFE
            );
            color: white;
        }
        QPushButton:disabled {
            background-color: #DFE6E9;
            color: #B2BEC3;
            border-color: #F0F0F0;
        }
        QFrame#imageFrame {
            background-color: #FFFBF0;
            border: 5px dashed #FDCB6E;
            border-radius: 22px;
        }
        QLabel#imagePreview {
            background-color: transparent;
            border: none;
            font-size: 17pt;
            color: #B2BEC3;
        }
        QScrollArea {
            border: none;
            background: transparent;
        }
        QScrollArea > QWidget > QWidget {
            background: transparent;
        }
    )");
}

} // namespace AppStyles

#endif // STYLES_H
