class InstructionDescription(object):
	def __init__(self, opcode, instruction_size):
		self.opcode = opcode
		self.instruction_size = instruction_size

def is_instruction_supported(line):
	return line[26] == 'X'

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

def build_parse_table(data):
	print 'static struct instruction_description opcode_to_insn_size[] = {'
	lines = data.split('\n')
	instruction_descriptions = []
	for line in lines:
		opcode_text = line[2:4]
		try:
			opcode = int(opcode_text, 16)
		except ValueError:
			continue

		if (not is_instruction_supported(line)):
			continue

		instruction_size = get_instruction_size(line)
		instruction_descriptions.append(InstructionDescription(opcode, instruction_size))

	instruction_descriptions = sorted(instruction_descriptions, key=lambda desc: desc.opcode)

	for i in range(len(instruction_descriptions)):
		# last entry should have no comma at the end
		if i == len(instruction_descriptions) - 1:
			print '\t{}'.format(instruction_descriptions[i].instruction_size)
		else:
			print '\t{},'.format(instruction_descriptions[i].instruction_size)

	print '};'


def main():
	with open('65xx Opcode List.txt', 'rb') as f:
		build_parse_table(f.read())

if __name__ == '__main__':
	main()