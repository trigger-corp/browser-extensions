"""Helpers for cli interaction"""
import sys
import math
from getpass import getpass


def ask_confirm(confirm_event):
	print "\n" + confirm_event.get('content')
	if confirm_event['cancellable']:
		user_input = raw_input("\nContinue? [Y/n]")
		return user_input.strip() == "" or user_input.lower().startswith("y")
	else:
		user_input = raw_input("\nPress enter to continue.")
		return True

def ask_question(question_event):
	properties = question_event.get('schema').get('properties')

	response = {}
	for item_name, item_schema in properties.items():
		if item_schema.get('type') == 'string':
			title = item_schema.get('title')
			description = item_schema.get('description', '')
			
			print "\n" + description

			if 'enum' in item_schema:
				choices = item_schema['enum']
				lines = [" (%d) %s" % (i + 1, c) for i, c in enumerate(choices)]

				print "\n".join(lines)

				prompt = "Enter a choice between 1-%d: " % len(choices)
				
				choice = None
				while choice is None:
					try:
						inp = raw_input("\n" + prompt)
						n = int(inp.strip())

						if not (1 <= n <= len(choices)):
							raise ValueError

						choice = n
					except ValueError:
						print "Invalid choice"

				answer = choices[choice - 1]

			elif '_password' in item_schema:
				answer = getpass('%s: ' % title)

			else:
				answer = raw_input('%s: ' % title)

			response[item_name] = answer

	return response

def _print_progress(width, message, fraction):
	capped_fraction = min(max(0, fraction), 1)
	filled = int(math.floor(width * capped_fraction))
	unfilled = width - filled
	
	sys.stdout.write('%30s [%s%s]' % (
		message, '=' * filled, ' ' * unfilled
	))
	sys.stdout.flush()

def start_progress(progress_event, width=50):
	_print_progress(width, progress_event['message'], 0)

def end_progress(progress_event, width=50):
	sys.stdout.write('\n')

def progress_bar(progress_event, width=50):
	# called with
	fraction = progress_event['fraction']
	message = progress_event['message']

	# go back to start of line so can redraw on top of this
	sys.stdout.write('\r')

	_print_progress(width, message, fraction)
