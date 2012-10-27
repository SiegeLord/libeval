/*
** generic hash table library
** Copyright (C) 2006  Jeffrey S. Dutky
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef struct hashtable_struct hashtable;

/* create an empty hashtable with p_size slots, the p_hash() function is used
** to generate a slot number (hash code) given a key value, the p_comp()
** function is used to compare two key values, returning -1 if key1 is less
** than key2, 0 if the two keys are equal and 1 if key1 is greater than key2,
** the p_kdel() function is called when a key must be deleted, the p_vdel()
** function is called when a value must be deleted. The ht_create() function
** returns a pointer to an empty hashtable on success, NULL on failure. */
hashtable *ht_create(unsigned long p_size,
	unsigned int (*p_hash)(void *key),
	int (*p_comp)(void *key1,void *key2),
	void (*p_kdel)(void *key),
	void (*p_vdel)(void *val));

/* delete the entire table, calling the kdel and vdel functions
** provided to ht_create as needed, return 0 (zero) on success,
** non-zero otherwise */
void ht_delete(hashtable *p_ht);

/* insert key and value into table, return 0 (zero) on success,
** non-zero otherwise */
int ht_insert(hashtable *p_ht, void *p_key, void *p_val);

/* remove key and value from table, return 0 (zero) on success,
** non-zero otherwise */
int ht_remove(hashtable *p_ht, void *p_key, void **p_val);

/* lookup key in table, return 0 (zero) if found, non-zero otherwise */
int ht_lookup(hashtable *p_ht, void *p_key, void **p_val);

/* iterate over the entries in the hashtable calling the provided function
** (p_func) for each entry. The p_func() function takes three parameters:
** the slot number of the current key-value pair, the key and the value.
** if p_func returns non-zero the iteration will be aborted and the p_rv
** parameter will be set to the value returned by p_func. The ht_iterate()
** function returns 0 (zero) on success, non-zero on failure. */
int ht_iterate(hashtable *p_ht, int (*p_func)(unsigned long slot,
	void *key, void *val), int *p_rv);

#endif
