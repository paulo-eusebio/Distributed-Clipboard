
# Report

## Graphical Representation of the architecture and components of the system

--

## The way thread management is performed should be described.
### For each process student should list all thread, describe their funcionality and explaining how they are created

--

## What data structures are shared among threads and how synchronization is performed.

--

## How local communication is performed

-- Sockets UNIX

## How remote communication is performed

-- TCP, sockets unix

## How data is replicated among the various clipboards

-- TCP, sockets unix, protocolo especifico




LEMBRETES:

Pensar nesta lógica,
	Se mexer numa region ----> write-read lock
	Se mexer numa socket\fd ---> lock
	Se mexer na lista de fd ---> lock

Nao é preciso dar lock nas readroutins de um socket tcp porque não é possivel acontecer duas threads diferentes querem dar um read do mesmo clpboard

É preciso dar lock nas writeroutines para um pai pq pode haver varias threads a querer mandar um 'c' ou 'n' para o pai

Dizer que protegemos para o caso de haver \0s pelo meio

Porquê mutexes em vez semaphores?? (overhead menor?) nao sei se vale a pena falar nisto

De forma a prevenir atrasos, só fazemos lock dos fd's dos filhos (send to children) quando o clip é single
porque nunca nunca vai acontecer, num not single querer fazer sendTochildren em paraello, porque o que recee da app manda sempre para cima 1º

Se eu tenho pai, e o gajo morre enquanto lhe estava a mandar a msg upstream, a mensagem perde-se i guess (rebuscado)

Onde usamos mutexes, dar exemplo de pq dava merda sem eles
Onda usamos readlock dar exemplo de porque é que da jeito

	- Porquê mutex fora do add();?
		-Porque nao queremos percorrer uma lista, à qual podem ter sido removidas cenas no instante em que percorremos a lista
		-ex, quero adicionar no fim da lista, e o endereço do ultimo nó, mas no momemtn em que vou por o ultimo nó a 		             apontra para o novo nó, o ultimo nó deixa de existir	
	- Porquê mutex para o pai
		- Recebo copy de varias apps ao mesmo tempo e quero mandar para cima, nao posso deixar gente aceder ao mesmo fd ao mesmo tempo
	- Porque não read lock para o pai?
		- Só ha uma thread a receber cenas do pai, por isso não vão haver reads concorrentes do socket que liga ao pai
	- Porque nao readlock quando recebo um 'n' e sou o pai?
		- Nao posso por um readlock antes do sendtochildren, porque não posso correr o risco de outra thread alterar a região
		- Antes de eu enviar a atualização para os filhos
		- Se uma app quiser um paste terá que esperar, no entanto é preciso um read lock, caso varias apps façam paste ao mesmo tmepo
	- Porquê lock dentro do sendToChildren\parent
		- Eu não quero que ninguem escreva para aquele socket até eu o largar, posso receber um m de dois clips diferentes
		- querer mandar para os meus filhos (sendToChildren) tenho que garantir que só manda um de cada vez
	- Porque é que não damos lock do fd da app?
		- Só há uma thread a escrever e a ler para a mesma app, nunca duas threads diferentes querem aceder ao mesmo socket de uma app

	- Stdin, lock fora do for() para ser mais rápido (source github)

	- SendToChilder	
		- SendToChildren, lock do fd, pode acontecer (na verdade só acontece no topo), recebo um 'n' dum filho e um 'c' da app e vou
		fazer sendtochildren ao mesmo tempo
		- ACHO QUE ISTO NAO VAI ACONTECER, PORQUE ESTAMOS A DAR LOCK DO ACESSO A LISTA DE CHILDREN, POR ISSO NUNCA 2 THREADS DIFERENTES
		VAO CONSEGUIR ESCREVER PO MESMO SOCKET, a que chega primeiro vai ter de dar a volta toda, largar o mutex e so deopis é que a 
		segunda consegue escrever para o filho
	
	



