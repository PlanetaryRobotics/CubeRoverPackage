# print('__file__={0:<35} \n | __name__={1:<20}\n | __package__={2:<20}\n'.format(__file__,__name__,str(__package__)))
import inspect
import os
import sys
__THIS_FILE_DIR = os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))
__TELEOP_BACKEND_DIR = os.path.dirname(__THIS_FILE_DIR)
# import GroundSoftware.Backend

import unittest
# import ...teleop_backend_main
from teleop_backend import teleop_backend_main
# from ...teleop_backend import teleop_backend_main
from unittest import mock
from unittest.mock import MagicMock
from teleop_backend.database.multiprocess_db_handler import MultiprocessDbHandler

class TestDBHandler(unittest.TestCase):
    def setUp(self):
        self.mock_db_collection = mock.MagicMock()
        self.recently_updated_lookup_ids = [14]
        self.mock_output_queue = mock.MagicMock()

        return super().setUp()
    
    def tearDown(self):
        return super().tearDown()

    def test_1(self):
        self.assertEqual(0,0)
        # mock_DBHandler = unittest.mock
        # mock_DBHandler.mock_calls


    # def test_new_command_from_db_ouput(self):
    #     mock_db_collection = mock.MagicMock()
    #     new_cmd_dict = mock.MagicMock()
    #     MultiprocessDbHandler.new_command_from_db_output(new_cmd_dict)
    #     new_cmd_dict.assert_called_once()

    def test_2(self):
        # mock_db_collection = mock.MagicMock()
        # recently_updated_lookup_ids = [14]
        # mock_output_queue = mock.MagicMock()
        test_command = {'args': {'Angle': 111, 'Images': 4, 'Speed': 11}, 'lookupID': 13, 'name': 'DriveLeftCommand'}
        mock_new_commands = [test_command]
        self.mock_db_collection.changes.get_all_new.return_value = mock_new_commands
        MultiprocessDbHandler.db_new_commands_handler(self.mock_db_collection,
                                                      self.recently_updated_lookup_ids,
                                                      self.mock_output_queue)
        self.mock_output_queue.put.assert_called_once()                                              
        # self.mock_output_queue.assert_called_once_with(MultiprocessDbHandler.new_command_from_db_output(test_command))

    def test3(self):
        # self.recently_updated_lookup_ids = [15]
        test_command = {'args': {'Angle': 111, 'Images': 4, 'Speed': 11}, 'lookupID': 13, 'name': 'DriveLeftCommand'}
        test_command_skipped = {'args': {'Angle': 13, 'Images': 12, 'Speed': 3}, 'lookupID': 14, 'name': 'DriveRightCommand'}
        mock_new_commands = [test_command_skipped]
        self.mock_db_collection.changes.get_all_new.return_value = mock_new_commands
        MultiprocessDbHandler.db_new_commands_handler(self.mock_db_collection,
                                                      self.recently_updated_lookup_ids,
                                                      self.mock_output_queue)
        self.mock_output_queue.assert_not_called()

    # def test4(self):
    #     test_command = {'args': {'Angle': 111, 'Images': 4, 'Speed': 11}, 'lookupID': 13, 'name': 'DriveLeftCommand'}
    #     mock_new_commands = [test_command]
    #     self.mock_db_collection.changes.get_all_new.return_value = mock_new_commands
    #     MultiprocessDbHandler.db_new_commands_handler(self.mock_db_collection,
    #                                                   self.recently_updated_lookup_ids,
    #                                                   self.mock_output_queue)

    #     self.mock_output_queue.assert_called_once()

    # def test5(self):
    #     test_command = {'args': {'Angle': 111, 'Images': 4, 'Speed': 11}, 'lookupID': 13, 'name': 'DriveLeftCommand'}
    #     mock_new_commands = [test_command]
    #     self.mock_db_collection.changes.get_all_new.return_value = mock_new_commands
    #     MultiprocessDbHandler.db_new_commands_handler(self.mock_db_collection,
    #                                                   self.recently_updated_lookup_ids,
    #                                                   self.mock_output_queue)

        self.mock_output_queue.assert_called_once()


    # def new_command_from_db_output(new_cmd_dict):

    

if __name__ == '__main__':
    unittest.main()
