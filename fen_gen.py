import chess, random

def random_fen():
	board = chess.Board()

	while True:
		try:
			moves = random.randint(0, 50)
			for _ in range(moves):
				move = random.choice(list(board.legal_moves))
				board.push(move)
		except:
			pass
		else:
			break
	
	pseudo_legal_moves = [m.uci() for m in board.pseudo_legal_moves]
	pseudo_legal_moves.sort()

	return board.fen(), ' '.join(pseudo_legal_moves)

def main():
	num_fens = 100_000
	fens, moves = zip(*[random_fen() for _ in range(num_fens)])

	with open('fens.txt', 'w') as f:
		f.write('\n'.join(fens))
	
	with open('moves.txt', 'w') as f:
		f.write('\n'.join(moves))

main()