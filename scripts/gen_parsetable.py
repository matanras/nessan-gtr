class InstructionDescription(object):
	def __init__(self, opcode, instruction_size):
		self.opcode = opcode
		self.instruction_size = instruction_size

def is_instruction_supported(line):
	return len(line) > 26 and line[26] == 'X'

def get_opcode(line):
	opcode_text = line[2:4]
	opcode = int(opcode_text, 16)
	return opcode

def get_instruction_mnemonic_with_format(line):
	mnemonic_text = line[6:26].replace(' ', '')
	# remove *
	mnemonic_text = mnemonic_text.replace('*', '')
	# replace all nearlabel/abcd/ab with %X
	mnemonic_text = mnemonic_text.replace('nearlabel', '$%X')
	mnemonic_text = mnemonic_text.replace('abcd', '%X')
	mnemonic_text = mnemonic_text.replace('ab', '%X')
	mnemonic_text = mnemonic_text[:3] + ' ' + mnemonic_text[3:] if '%X' in mnemonic_text else mnemonic_text
	mnemonic_text += '\\n'
	return mnemonic_text

def get_instruction_size(line):
	operands_text = line[11:26].replace(' ', '')
	size = 1 # accounting for opcode byte
	operand = 0
	for expression in operands_text.split(','):
		stripped_expression = expression.replace('(', '')
		stripped_expression = stripped_expression.replace(')', '')
		stripped_expression = stripped_expression.replace('$', '')
		stripped_expression = stripped_expression.replace('#', '')
		stripped_expression = stripped_expression.replace('[', '')
		stripped_expression = stripped_expression.replace(']', '')

		if not stripped_expression:
			break

		if 'X' == stripped_expression or 'Y' == stripped_expression or 'S' == stripped_expression:
			continue
		elif stripped_expression == 'nearlabel':
			size += 1
		elif stripped_expression == 'farlabel':
			size += 2
		else:
			size += len(stripped_expression) / 2
			operand = int(stripped_expression, 16)


	return size

def build_opcode_to_insn_size(data):
	print 'static uint8_t opcode_to_insn_size[] = {'
	lines = data.split('\n')
	instruction_descriptions = []
	for line in lines:
		if (not is_instruction_supported(line)):
			continue

		try:
			opcode = get_opcode(line)
		except ValueError:
			continue

		instruction_size = get_instruction_size(line)
		instruction_descriptions.append(InstructionDescription(opcode, instruction_size))

	instruction_descriptions = sorted(instruction_descriptions, key=lambda desc: desc.opcode)

	for i in range(len(instruction_descriptions)):
		# last entry should have no comma at the end
		if i == len(instruction_descriptions) - 1:
			print '\t[{}] = {}'.format(hex(instruction_descriptions[i].opcode),
									   instruction_descriptions[i].instruction_size)
		else:
			print '\t[{}] = {},'.format(hex(instruction_descriptions[i].opcode),
										instruction_descriptions[i].instruction_size)

	print '};'
	print ''

def build_opcode_to_mnemonic(data):
	print 'static char *opcode_to_mnemonic[] = {'
	lines = data.split('\n')
	instruction_mnemonics = []
	for line in lines:
		if (not is_instruction_supported(line)):
			continue

		try:
			opcode = get_opcode(line)
		except ValueError:
			continue

		mnemonic = get_instruction_mnemonic_with_format(line)
		instruction_mnemonics.append((opcode, mnemonic))

	instruction_mnemonics = sorted(instruction_mnemonics, key=lambda t: t[0])

	for i in range(len(instruction_mnemonics)):
		# last entry should have no comma at the end
		if i == len(instruction_mnemonics) - 1:
			print '\t[{}] = "{}"'.format(hex(instruction_mnemonics[i][0]),
									   instruction_mnemonics[i][1])
		else:
			print '\t[{}] = "{}",'.format(hex(instruction_mnemonics[i][0]),
										instruction_mnemonics[i][1])

	print '};'

def build_tables(data):
	build_opcode_to_insn_size(data)
	build_opcode_to_mnemonic(data)


def main():
	with open('65xx Opcode List.txt', 'rb') as f:
		build_tables(f.read())

if __name__ == '__main__':
	main()