import sys
from PyQt6.QtWidgets import QApplication, QLabel, QWidget, QMainWindow


class MainApp(QMainWindow):
    def __init__(self):
        super().__init__(parent=None)
        self.setWindowTitle("Open CV Test")
        self.setFixedSize(640, 640)
        central_widget = QWidget(self)


if __name__ == "__main__":
    app = QApplication([])
    window = MainApp()
    window.show()
    sys.exit(app.exec())
