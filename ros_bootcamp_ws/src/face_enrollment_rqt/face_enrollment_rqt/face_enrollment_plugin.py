import os
from python_qt_binding import loadUi
from python_qt_binding.QtWidgets import QWidget, QVBoxLayout, QPushButton, QLineEdit, QLabel, QListWidget, QMessageBox
from python_qt_binding.QtCore import Qt
from rqt_gui_py.plugin import Plugin

from rclpy.node import Node
from bootcamp_vision_cpp.srv import EnrollFace, ListFaces, DeleteFace


class FaceEnrollmentPlugin(Plugin):
    def __init__(self, context):
        super(FaceEnrollmentPlugin, self).__init__(context)
        self.setObjectName('FaceEnrollmentPlugin')

        # Create the widget
        self._widget = QWidget()
        self._widget.setWindowTitle('Face Enrollment')
        
        # Create layout
        layout = QVBoxLayout()
        
        # Name input
        self._name_label = QLabel('Name:')
        layout.addWidget(self._name_label)
        
        self._name_input = QLineEdit()
        self._name_input.setPlaceholderText('Enter name to enroll...')
        layout.addWidget(self._name_input)
        
        # Enroll button
        self._enroll_btn = QPushButton('Enroll Face')
        self._enroll_btn.clicked.connect(self._on_enroll_clicked)
        layout.addWidget(self._enroll_btn)
        
        # Status label
        self._status_label = QLabel('Ready')
        self._status_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(self._status_label)
        
        # Enrolled faces list
        self._list_label = QLabel('Enrolled Faces:')
        layout.addWidget(self._list_label)
        
        self._faces_list = QListWidget()
        layout.addWidget(self._faces_list)
        
        # Refresh and Delete buttons
        self._refresh_btn = QPushButton('Refresh List')
        self._refresh_btn.clicked.connect(self._refresh_faces)
        layout.addWidget(self._refresh_btn)
        
        self._delete_btn = QPushButton('Delete Selected')
        self._delete_btn.clicked.connect(self._on_delete_clicked)
        layout.addWidget(self._delete_btn)
        
        self._widget.setLayout(layout)
        
        if context.serial_number() > 1:
            self._widget.setWindowTitle(
                self._widget.windowTitle() + (' (%d)' % context.serial_number()))
        
        context.add_widget(self._widget)
        
        # ROS node
        self._node = context.node
        
        # Service clients
        self._enroll_client = self._node.create_client(EnrollFace, '/vision/enroll_face')
        self._list_client = self._node.create_client(ListFaces, '/vision/list_faces')
        self._delete_client = self._node.create_client(DeleteFace, '/vision/delete_face')
        
        # Wait for services
        self._status_label.setText('Waiting for services...')
        
        # Refresh the list on startup
        self._refresh_faces()

    def _on_enroll_clicked(self):
        name = self._name_input.text().strip()
        
        if not name:
            self._status_label.setText('Error: Name cannot be empty')
            return
        
        self._status_label.setText(f'Enrolling {name}...')
        self._enroll_btn.setEnabled(False)
        
        # Call service
        if not self._enroll_client.wait_for_service(timeout_sec=1.0):
            self._status_label.setText('Error: Service not available')
            self._enroll_btn.setEnabled(True)
            return
        
        request = EnrollFace.Request()
        request.name = name
        
        future = self._enroll_client.call_async(request)
        future.add_done_callback(self._enroll_response_callback)
    
    def _enroll_response_callback(self, future):
        try:
            response = future.result()
            if response.success:
                self._status_label.setText(f'✓ {response.message}')
                self._name_input.clear()
                self._refresh_faces()
            else:
                self._status_label.setText(f'✗ {response.message}')
        except Exception as e:
            self._status_label.setText(f'Error: {str(e)}')
        finally:
            self._enroll_btn.setEnabled(True)
    
    def _refresh_faces(self):
        if not self._list_client.wait_for_service(timeout_sec=1.0):
            self._status_label.setText('Error: List service not available')
            return
        
        request = ListFaces.Request()
        future = self._list_client.call_async(request)
        future.add_done_callback(self._list_response_callback)
    
    def _list_response_callback(self, future):
        try:
            response = future.result()
            self._faces_list.clear()
            for name in response.names:
                self._faces_list.addItem(name)
            
            if response.count == 0:
                self._status_label.setText('No faces enrolled')
            elif self._status_label.text() == 'Waiting for services...':
                self._status_label.setText('Ready')
        except Exception as e:
            self._status_label.setText(f'Error listing faces: {str(e)}')
    
    def _on_delete_clicked(self):
        selected_items = self._faces_list.selectedItems()
        if not selected_items:
            self._status_label.setText('Select a face to delete')
            return
        
        name = selected_items[0].text()
        
        # Confirm deletion
        reply = QMessageBox.question(
            self._widget, 
            'Confirm Deletion',
            f'Delete face "{name}"?',
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )
        
        if reply != QMessageBox.Yes:
            return
        
        self._status_label.setText(f'Deleting {name}...')
        
        if not self._delete_client.wait_for_service(timeout_sec=1.0):
            self._status_label.setText('Error: Delete service not available')
            return
        
        request = DeleteFace.Request()
        request.name = name
        
        future = self._delete_client.call_async(request)
        future.add_done_callback(self._delete_response_callback)
    
    def _delete_response_callback(self, future):
        try:
            response = future.result()
            if response.success:
                self._status_label.setText(f'✓ {response.message}')
                self._refresh_faces()
            else:
                self._status_label.setText(f'✗ {response.message}')
        except Exception as e:
            self._status_label.setText(f'Error: {str(e)}')

    def shutdown_plugin(self):
        self._enroll_client.destroy()
        self._list_client.destroy()
        self._delete_client.destroy()

    def save_settings(self, plugin_settings, instance_settings):
        pass

    def restore_settings(self, plugin_settings, instance_settings):
        pass
