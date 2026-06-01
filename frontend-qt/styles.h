#ifndef STYLES_H
#define STYLES_H

#include <QString>

namespace AppStyles {

inline QString applicationStyleSheet()
{
    return QStringLiteral(R"(
        QWidget {
            background-color: #E8F4FD;
            color: #2D3436;
            font-family: "Comic Sans MS", "Segoe UI", sans-serif;
            font-size: 16pt;
        }
        QLabel#titleLabel {
            font-size: 26pt;
            font-weight: bold;
            color: #E17055;
        }
        QLineEdit {
            background-color: #FFFFFF;
            border: 3px solid #74B9FF;
            border-radius: 12px;
            padding: 12px 16px;
            font-size: 16pt;
            min-height: 28px;
        }
        QLineEdit:focus {
            border-color: #FD79A8;
        }
        QTextEdit {
            background-color: #FFFFFF;
            border: 3px solid #55EFC4;
            border-radius: 12px;
            padding: 12px;
            font-size: 15pt;
        }
        QPushButton {
            background-color: #FF7675;
            color: white;
            border: none;
            border-radius: 16px;
            padding: 14px 28px;
            font-size: 17pt;
            font-weight: bold;
            min-height: 52px;
            min-width: 180px;
        }
        QPushButton:hover {
            background-color: #E84393;
        }
        QPushButton:pressed {
            background-color: #D63031;
        }
        QPushButton#secondaryButton {
            background-color: #74B9FF;
        }
        QPushButton#secondaryButton:hover {
            background-color: #0984E3;
        }
        QPushButton#successButton {
            background-color: #55EFC4;
            color: #2D3436;
        }
        QPushButton#successButton:hover {
            background-color: #00B894;
            color: white;
        }
        QPushButton:disabled {
            background-color: #B2BEC3;
            color: #636E72;
        }
        QLabel#imagePreview {
            background-color: #FFFFFF;
            border: 4px dashed #FDCB6E;
            border-radius: 16px;
        }
    )");
}

} // namespace AppStyles

#endif // STYLES_H
