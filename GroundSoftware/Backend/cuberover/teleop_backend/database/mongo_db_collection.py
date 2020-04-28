#!/usr/bin/env python3
"""
    Wrapper Class for Interfacing with a MongoDB Collection in the Database
    and Accessing All Changed Documents.

    Author: Connor W. Colombo, CubeRover
    Last Updated: 6/19/2019
"""

import pymongo
import enum


class MongoDbState(enum.Enum):
    NOT_SENT = "NOT_SENT"
    SUCC_SENT_TO_YAMCS = "SUCC_SENT_TO_YAMCS"
    SUCC_QUEUED_BY_YAMCS = "SUCC_QUEUED_BY_YAMCS"
    SUCC_RELEASED_BY_YAMCS = "SUCC_RELEASED_BY_YAMCS"
    SUCC_SENT_BY_YAMCS = "SUCC_SENT_BY_YAMCS"
    SUCC_EXEC = "SUCC_EXEC"
    FAIL = "FAIL"



class MongoDbCollection:
    # Selector indicating which fields of a document in a collection are intended for the backend:
    BACKEND_FIELD_SELECTOR = {'lookupID': 1, 'name': 1, 'args': 1, 'type': 1, 'opcode': 1}

    class Names(enum.Enum):
        """Enumeration of all valid collection names."""
        COMMANDS = 'Commands'
        IMAGES = 'Images'

    # Iteration wrapper for pymongo's collection cursor.
    # Allows for
    class _ChangeIterator:
        """ Iterable wrapper for pymongo's iterable collection cursor which
        extracts the payload intended for the backend from a command document.

        This exists within its own class instead of __iter__ and __next__
        methods of the outer Collection class since the iterator in the outer
        class is used to iterate through all documents in the collection.
        """

        def __init__(self, collection):
            self.collection = collection
            # Watch for insert, replace, and update operations - not delete:
            self.cursor = collection.watch(full_document='updateLookup', pipeline=[
                {'$match': {'operationType': {'$ne': 'delete'}}},
                {'$project': {'fullDocument': MongoDbCollection.BACKEND_FIELD_SELECTOR}}
            ])

        def __iter__(self):
            return self

        def __next__(self):
            return next(self.cursor)['fullDocument']

        def try_next(self):
            change = self.cursor.try_next()
            return change['fullDocument'] if change is not None else None

        def get_all_new(self):
            all_new = []

            change = self.cursor.try_next()
            while change is not None:
                all_new.append(change['fullDocument'])
                change = self.cursor.try_next()

            return all_new

    def __init__(self, collection=Names.COMMANDS):
        assert isinstance(collection, MongoDbCollection.Names)

        self.client = pymongo.MongoClient(
            'mongodb+srv://CubeRoverAdmin:RedRover@devcluster-3thor.mongodb.net/test?retryWrites=true&w=majority')
        self.db = self.client['test']
        self.collection = self.db[collection.value]
        self.changes = MongoDbCollection._ChangeIterator(self.collection)

    def __iter__(self):
        """ Get iterator for the backend payloads of each document in the
            collection.
            NOTE: This iteration won't necessarily be in any particular or
            consistent order.
        """
        self.cursor = self.collection.find({}, MongoDbCollection.BACKEND_FIELD_SELECTOR)
        return self.cursor

    def __next__(self):
        """Iterate through all documents in the collection"""
        return next(self.cursor)

    def count(self):
        """ Get an accurate count across entire collection across DB, even if
            collection is sharded or documents are orphaned.
        """
        counter = self.collection.aggregate([
            {'$group': {'_id': None, 'numDocs': {'$sum': 1}}},
            {'$project': {'_id': 0}}
        ])
        # Extract result document from aggregation cursor. Extract count from document:
        return [x for x in counter][0]['numDocs']

    def write(self, doc):
        """ Adds the given document to the DB collection. Sets the lookupID to
            the increment of the accurate count of all documents across the
            database.
        """
        doc['lookupID'] = self.count() + 1
        self.collection.insert_one(doc)

    def fetch(self, lid, include_everything=False):
        """ Returns the contents of the backend payload of the document which
            has the given lookupID, `lid`.
            lookupID can be a MongoDB query selector.
            Alternatively, `includeEverything` can be flagged if more data
            (not just data earmarked for the backend) is desired.
        """
        selector = {'_id': 0} if include_everything else MongoDbCollection.BACKEND_FIELD_SELECTOR
        if isinstance(lid, dict):  # if query selector given
            csr = self.collection.find({'lookupID': lid}, selector)
            return [c for c in csr]  # return as a list (expand the cursor)
        else:
            return self.collection.find_one({'lookupID': lid}, selector)

    def update(self, lid, doc):
        """ Updates the contents of the document with the given lookupID, `lid`,
        overwriting any fields which are given in the given doc.
        lookupID can be a MongoDB query selector.
        """
        if isinstance(lid, dict):  # if query selector given
            self.collection.update_many({'lookupID': lid}, {'$set': doc}, upsert=False)
        else:
            self.collection.update_one({'lookupID': lid}, {'$set': doc}, upsert=False)
