import sqlite3
import csv

db = sqlite3.connect('macvendor.db')
cur = db.cursor()
cur.execute('CREATE TABLE IF NOT EXISTS macvendor (oui TEXT PRIMARY KEY, vendor TEXT)')

with open('oui.csv', 'r', encoding='utf-8') as f:
    reader = csv.DictReader(f)
    for row in reader:
        oui = row['Assignment'].strip().replace(':', '')  # используем Assignment
        vendor = row['Organization Name'].strip()
        if oui and vendor:
            cur.execute('INSERT OR REPLACE INTO macvendor (oui, vendor) VALUES (?, ?)', (oui, vendor))

db.commit()
db.close()