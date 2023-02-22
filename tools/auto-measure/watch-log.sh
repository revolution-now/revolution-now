#!/bin/bash
watch "cat $1 | sort | uniq -c | sort -nr"