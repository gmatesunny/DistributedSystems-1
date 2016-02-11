#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

using namespace std;
using std::vector;

// node struct
// x means x axes location
// y means y axes location
// e means energy
// leader means the leader for this node, initially it is its own
// elected means has this node been elected in the current level
// remaining means is this node still a leader or has been merged to another component
// dead means has this node run out of energy(less than minimal budget)
// visited means has this node been boardcasted

struct node
{
	double x, y, e;
	int id, leader;
	bool elected, remaining, dead, visited;
};

// start and end terminal of the edge

struct edge
{
	int start, end;
};

// calculate the distance between two different nodes

double dis(node n1, node n2)
{
	return sqrt(pow(n1.x - n2.x, 2) + pow(n1.y - n2.y, 2));
}

// output the base-stations

void print_leaders(vector<node> &nodes, int n, ofstream &fout)
{
	int i;
	vector<int> leaders;
	vector<int>::iterator it;

	fout << "bs ";
	for (i = 0; i < n; i++)
	{
		if (nodes[i].remaining)
		{
			leaders.push_back(nodes[i].id);
		}
	}
	for (it = leaders.begin(); it < leaders.end(); it++)
	{
		if (it < leaders.end() - 1)
		{
			fout << *it << ",";
		}
		else
		{
			fout << *it;
		}
	}
	fout << endl;
}

// change the old leader to the new leader, which in fact is the process of merging two components

void change_leader(int new_leader, int old_leader, vector<node> &nodes, int n)
{
	int i;

	for (i = 0; i < n; i++)
	{
		if (nodes[i].leader == old_leader)
		{
			nodes[i].leader = new_leader;
		}
	}
}

// to find the index of a node in the vector container

int find_nodes_id(int id, vector<node> &nodes, int n)
{
	int i;

	for (i = 0; i < n; i++)
	{
		if (nodes[i].id == id)
		{
			return i;
		}
	}
	return -1;
}

// broadcast function
// queue is used as a queue for BFS to traverse the network
// once find a connected node, add this node to the queue and consume the energy of current sender

void bcast(int sender, vector<node> &nodes, vector<vector<bool> > &hash, vector<vector<double> > &graph, int n, ofstream &fout)
{
	int i;
	vector<int> queue(n);
	vector<int>::iterator head, end;

	head = queue.begin();
	*head = sender;
	end = head;
	while (head <= end)
	{
		for (i = 0; i < n; i++)
		{
			if (hash[*head][i] && !nodes[i].visited && !nodes[i].dead)
			{
				nodes[i].visited = true;
				nodes[*head].e = nodes[*head].e - graph[*head][i] * 1.2;
				fout << "data from " << nodes[*head].id << " to " << nodes[i].id << ", energy:" << nodes[*head].e << endl;
				*++end = i;
			}
		}
		head++;
	}
}

// once there is a node "dead", rebuild the MST

void re_mst(vector<node> &nodes, vector<vector<bool> > &hash, vector<vector<double> > &graph, int n)
{
	int i, j, merge;

	vector<double> low_cost(n);
	vector<edge> link_to(n);

	// for the active node, change its leader to itself
	// and the "leader" attribution is still remaining

	for (i = 0; i < n; i++)
	{
		if (!nodes[i].dead)
		{
			nodes[i].leader = i;
			nodes[i].remaining = true;
		}
	}

	// clean the hash table for MST
	// if hash[i][j]==true, which means the edge of node i and j is a edge of MST

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			hash[i][j] = false;
		}
	}

	// set merging times to 1 for the first level

	merge = 1;
	while (true)
	{

		// if the merging time of last level is more than 0
		// then reset it to 0
		// otherwise, break the loop

		if (merge > 0)
		{
			merge = 0;
		}
		else
		{
			break;
		}

		// initialize the vector containers: low_cost and link_to
		// low_cost[i] stores the shortest length of edge which links to the component which contains node i 
		// link_to[i] stores the start and end node of this edge

		for (i = 0; i < n; i++)
		{
			low_cost[i] = 10;
			link_to[i].start = -1;
			link_to[i].end = -1;
		}

		// enumerate two different nodes
		// if they belongs to different components and both active
		// then compare the distance of two nodes with corresponding low_cost[i]
		// if the distance is smaller than low_cost[i]
		// then record the start and end terminals of this edge and update the low_cost[i]

		for (i = 0; i < n; i++)
		{
			for (j = 0; j < n; j++)
			{
				if (nodes[i].leader != nodes[j].leader && !nodes[i].dead && !nodes[j].dead)
				{
					if (graph[i][j] < low_cost[nodes[i].leader])
					{
						low_cost[nodes[i].leader] = graph[i][j];
						link_to[nodes[i].leader].end = j;
						link_to[nodes[i].leader].start = i;
					}
				}
			}
		}

		// if there are edges found between two components, merge these two components
		// change the leader of the component which has "smaller leader" to the "bigger leader"

		for (i = 0; i < n; i++)
		{
			if (link_to[i].end != -1)
			{
				int current = nodes[i].leader;
				int target = nodes[link_to[i].end].leader;

				if (current != target)
				{
					merge++;
					hash[link_to[i].start][link_to[i].end] = true;
					hash[link_to[i].end][link_to[i].start] = true;
					if (nodes[current].id < nodes[target].id)
					{
						change_leader(target, current, nodes, n);
					}
					else if (nodes[current].id > nodes[target].id)
					{
						change_leader(current, target, nodes, n);
					}
				}
			}
		}
	}
}

int main(int argc, char* args[])
{
	int n, limit, i, j, merge, sender, node_down;
	string order;
	char ch;

	ifstream fin(args[1]);
	//ifstream fin("input.txt");
	ofstream fout("log.txt");
	n = 0;

	// limit means the minimal energy budget of a node

	fin >> limit;
	node tempnode;
	vector<node> nodes;
	
	// input the nodes

	while (fin.peek() != EOF)
	{
		order = "";
		for (i = 0; i < 4; i++)
		{
			fin >> ch;
			order = order + ch;
		}
		if (order == "node")
		{
			fin >> tempnode.id >> ch >> tempnode.x >> ch >> tempnode.y >> ch >> tempnode.e;
			tempnode.leader = n;
			tempnode.elected = false;
			tempnode.remaining = true;
			tempnode.dead = false;
			tempnode.visited = false;
			nodes.push_back(tempnode);
			n++;
		}
		if (order == "bcst")
		{
			break;
		}
	}

	// low_cost[i] stores the shortest length of edge which links to the component which contains node i 
	// link_to[i] stores the start and end node of this edge
	// graph stores the distance of two different nodes
	// hash[i][j] means whether the edge of node i and j belongs to MST or not

	vector<double> low_cost(n, 0);
	vector<edge> link_to(n);
	vector<vector<double> > graph(n, vector<double> (n));
	vector<vector<bool> > hash(n, vector<bool> (n));
	merge = 1;

	// calculate the distance for all nodes
	// fill in the hash table with false at first

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			graph[i][j] = dis(nodes[i], nodes[j]); 
			hash[i][j] = false;
		}
	}
	while (true)
	{

		// if the merging time of last level is more than 0
		// then reset it to 0 and print the base-station of last level
		// otherwise, break the loop

		if (merge > 0)
		{
			print_leaders(nodes, n, fout);
			merge = 0;
		}
		else
		{
			break;
		}

		// initialize the vector containers: low_cost and link_to
		// before every level begins, reset every nodes to not elected

		for (i = 0; i < n; i++)
		{
			low_cost[i] = 10;
			link_to[i].start = -1;
			link_to[i].end = -1;
			nodes[i].elected = false;
		}

		// enumerate two different nodes
		// if they belongs to different components and both active
		// then compare the distance of two nodes with corresponding low_cost[i]
		// if the distance is smaller than low_cost[i]
		// then record the start and end terminals of this edge and update the low_cost[i]

		for (i = 0; i < n; i++)
		{
			for (j = 0; j < n; j++)
			{
				if (nodes[i].leader != nodes[j].leader)
				{
					if (graph[i][j] < low_cost[nodes[i].leader])
					{
						low_cost[nodes[i].leader] = graph[i][j];
						link_to[nodes[i].leader].end = j;
						link_to[nodes[i].leader].start = i;
					}
				}
			}
		}

		// if there are edges found between two components, merge these two components
		// change the leader of the component which has "smaller leader" to the "bigger leader"
		// every time a merging operation completes, output the start and end terminal of the edge

		for (i = 0; i < n; i++)
		{
			if (link_to[i].end != -1)
			{
				int current = nodes[i].leader;
				int target = nodes[link_to[i].end].leader;

				if (current != target)
				{
					merge++;
					fout << "added " << nodes[link_to[i].start].id << "-" << nodes[link_to[i].end].id << endl;
					hash[link_to[i].start][link_to[i].end] = true;
					hash[link_to[i].end][link_to[i].start] = true;
					if (nodes[current].id < nodes[target].id)
					{
						change_leader(target, current, nodes, n);
						nodes[current].remaining = false;
						nodes[target].elected = true;
					}
					else if (nodes[current].id > nodes[target].id)
					{
						change_leader(current, target, nodes, n);
						nodes[target].remaining = false;
						nodes[current].elected = true;
					}
				}
			}
		}

		// if merging time is more than 0, then output the newly elected leaders
		// sometimes, multiple parts will be merged into one part
		// so only output the nodes which have been elected and still remaining(alive)

		if (merge > 0)
		{
			for (i = 0; i < n; i++)
			{
				if (nodes[i].elected && nodes[i].remaining)
				{
					fout << "elected " << nodes[i].id << endl;
				}
			}
		}
	}
	
	if (order == "bcst")
	{
		for (i = 0; i < 4; i++)
		{
			fin >> ch;
		}
		fin >> sender;
		node_down = 0;

		while (sender > 0)
		{

			// find the index of sender in vector container

			sender = find_nodes_id(sender, nodes, n);

			if (!nodes[sender].dead)
			{

				// if there is any node down in last broadcast, then rebuild MST

				if (node_down > 0)
				{
					re_mst(nodes, hash, graph, n);
					node_down = 0;
				}
				nodes[sender].visited = true;

				// boardcast from sender node

				bcast(sender, nodes, hash, graph, n, fout);

				// if the energy of a node is less than the minimal budget, this node is dead
				// every edge to or from this specific dead node becomes 10(which means invaild/unconnected in this network)

				for (i = 0; i < n; i++)
				{
					if (nodes[i].e < limit && !nodes[i].dead)
					{
						nodes[i].dead = true;
						node_down++;
						fout << "node down " << nodes[i].id << endl;
						for (j = 0; j < n; j++)
						{
							graph[i][j] = 10;
							graph[j][i] = 10;
						}
					}
					nodes[i].visited = false;
				}
			}
			sender = -1;
			if (fin.peek() != EOF)
			{
				for (i = 0; i < 8; i++)
				{
					fin >> ch;
				}
				fin >> sender;
			}
		}
	}
	return 0;
}
