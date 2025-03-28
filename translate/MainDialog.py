import sys
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QTableView, 
    QTableWidget, QTableWidgetItem, QVBoxLayout, 
    QWidget, QLabel, QPushButton, QHBoxLayout
)
from PyQt5.QtCore import QAbstractTableModel, Qt

class CustomTableModel(QAbstractTableModel):
    def __init__(self, data):
        super().__init__()
        self._data = data

    def rowCount(self, parent=None):
        return len(self._data)

    def columnCount(self, parent=None):
        return len(self._data[0]) if self._data else 0

    def data(self, index, role=Qt.DisplayRole):
        if role == Qt.DisplayRole:
            return self._data[index.row()][index.column()]

    def setData(self, index, value, role=Qt.EditRole):
        if role == Qt.EditRole:
            self._data[index.row()][index.column()] = value
            self.dataChanged.emit(index, index)
            return True
        return False

    def flags(self, index):
        return Qt.ItemIsEditable | Qt.ItemIsEnabled | Qt.ItemIsSelectable

class Example(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("QTableView and QTableWidget Example")
        self.setGeometry(100, 100, 800, 600)

        # Main widget
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)

        # Layout
        layout = QVBoxLayout(self.central_widget)

        # QTableView
        self.table_view = QTableView()
        self.table_data = [["Row 1 Col 1", "Row 1 Col 2"],
                           ["Row 2 Col 1", "Row 2 Col 2"],
                           ["Row 3 Col 1", "Row 3 Col 2"]]
        
        self.table_model = CustomTableModel(self.table_data)
        self.table_view.setModel(self.table_model)
        layout.addWidget(QLabel("QTableView:"))
        layout.addWidget(self.table_view)

        # QTableWidget
        self.table_widget = QTableWidget(3, 2)  # 3 rows, 2 columns
        self.table_widget.setHorizontalHeaderLabels(["Column 1", "Column 2"])
        for i in range(3):
            for j in range(2):
                self.table_widget.setItem(i, j, QTableWidgetItem(f"Item {i+1}, {j+1}"))
        layout.addWidget(QLabel("QTableWidget:"))
        layout.addWidget(self.table_widget)

        # Buttons for adding and removing rows
        button_layout = QHBoxLayout()
        self.add_button = QPushButton("Add Row")
        self.add_button.clicked.connect(self.add_row)
        button_layout.addWidget(self.add_button)

        self.remove_button = QPushButton("Remove Row")
        self.remove_button.clicked.connect(self.remove_row)
        button_layout.addWidget(self.remove_button)

        layout.addLayout(button_layout)

    def add_row(self):
        # Add a new row to QTableView
        new_row = ["New Row Col 1", "New Row Col 2"]
        self.table_model._data.append(new_row)
        self.table_model.layoutChanged.emit()  # Notify the view to refresh

        # Add a new row to QTableWidget
        row_count = self.table_widget.rowCount()
        self.table_widget.insertRow(row_count)
        for j in range(2):
            self.table_widget.setItem(row_count, j, QTableWidgetItem(f"Item {row_count + 1}, {j + 1}"))

    def remove_row(self):
        # Remove the selected row from QTableView
        selected_index = self.table_view.currentIndex()
        if selected_index.isValid() and self.table_model.rowCount() > 0:
            self.table_model._data.pop(selected_index.row())
            self.table_model.layoutChanged.emit()  # Notify the view to refresh

        # Remove the selected row from QTableWidget
        selected_row = self.table_widget.currentRow()
        if selected_row >= 0:
            self.table_widget.removeRow(selected_row)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = Example()
    window.show()
    sys.exit(app.exec_())