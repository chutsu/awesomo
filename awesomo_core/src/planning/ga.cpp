#include "awesomo_core/planning/ga.hpp"


namespace awesomo {

void problem_setup(struct problem_data *p,
                   int nb_states,
                   int nb_inputs,
                   int nb_steps,
                   std::vector<double> cost_weights) {
  p->nb_states = nb_states;
  p->nb_inputs = nb_inputs;
  p->nb_steps = nb_steps;

  p->pos_init << 0.0, 0.0;
  p->pos_final << 0.0, 0.0;
  p->vel_init << 0.0, 0.0;
  p->vel_final << 0.0, 0.0;
  p->theta_init = 0.0;
  p->theta_final = 0.0;

  p->desired.resize(nb_states + nb_inputs, nb_steps);
  p->cost_weights = cost_weights;
}

GABitString::GABitString(void) {
  this->chromosome.clear();
  this->nb_time_steps = 0;
  this->score = 0.0;
}

int GABitString::configure(int nb_time_steps) {
  int nb_states;
  int nb_inputs;
  int n;

  // setup
  nb_states = 4;
  nb_inputs = 2;
  n = nb_states + nb_inputs;
  this->nb_time_steps = nb_time_steps;

  // initialize chromosome
  for (int i = 0; i < nb_time_steps; i++) {
    this->chromosome.push_back(randf(-2.0, 5.0));    // x
    this->chromosome.push_back(randf(-5.0, 5.0));    // vx
    this->chromosome.push_back(randf(0.0, 10.0));    // z
    this->chromosome.push_back(randf(-5.0, 5.0));    // vz
    this->chromosome.push_back(randf(-10.0, 10.0));  // az
    this->chromosome.push_back(randf(deg2rad(-90.0), deg2rad(90.0)));  // theta
  }
}

int GABitString::record(std::string file_path) {
  std::ofstream output_file;
  int m;

  // setup
  output_file.open(file_path);
  output_file << "time_step" << ",";
  output_file << "x" << ",";
  output_file << "vx" << ",";
  output_file << "z" << ",";
  output_file << "vz" << ",";
  output_file << "az" << ",";
  output_file << "theta" << std::endl;

  // record
  m = 6;  // states + inputs (4 + 2)
  for (int i = 0; i < this->nb_time_steps; i++) {
    output_file << i << ",";
    output_file << this->chromosome[i * m + 0] << ",";
    output_file << this->chromosome[i * m + 1] << ",";
    output_file << this->chromosome[i * m + 2] << ",";
    output_file << this->chromosome[i * m + 3] << ",";
    output_file << this->chromosome[i * m + 4] << ",";
    output_file << this->chromosome[i * m + 5] << std::endl;
  }

  // clean up
  output_file.close();

  return 0;
}


void GABitString::print(void) {
  for (int i = 0; i < this->chromosome.size(); i++) {
    std::cout << "[" << i << "]: " << this->chromosome[i] << std::endl;
  }
}

GAPopulation::GAPopulation(void) {
  this->individuals.clear();
}

int GAPopulation::configure(int nb_individuals, struct problem_data *data) {
  std::vector<double> desired;
  load_matrix(data->desired, desired);

  for (int i = 0; i < nb_individuals; i++) {
    GABitString bs;
    bs.configure(data->nb_steps);
    bs.chromosome = std::vector<double>(desired);
    this->individuals.push_back(bs);
  }
}

void GAPopulation::print(void) {
  for (int i = 0; i < this->individuals.size(); i++) {
    std::cout << "individual[" << i << "]: ";
    std::cout << "score: " << this->individuals[i].score << std::endl;
  }
}

GAProblem::GAProblem(void) {
  this->configured = false;
}

int GAProblem::configure(int max_generations,
                         int tournament_size,
                         double crossover_probability,
                         double mutation_probability) {

  // seed random
  srand(time(NULL));

  // fields
  this->max_generations = max_generations;
  this->tournament_size = tournament_size;
  this->crossover_probability = crossover_probability;
  this->mutation_probability = mutation_probability;
  this->configured = true;

  return 0;
}

int GAProblem::calculateDesired(struct problem_data *p) {
  double dx;
  VecX x(6);
  double m, c;

  // setup
  p->desired.block(0, p->nb_steps - 1, 6, 1) = x;

  // calculate line equation, gradient and intersect
  m = (p->pos_init(1) - p->pos_final(1)) / (p->pos_init(0) - p->pos_final(0));
  c = p->pos_init(1) - m * p->pos_init(0);

  // push initial x
  x(0) = p->pos_init(0);  // state - x
  x(1) = p->vel_init(0);  // state - vx
  x(2) = p->pos_init(1);  // state - z
  x(3) = p->vel_init(1);  // state - vz
  x(4) = p->thrust_init;  // input - az
  x(5) = p->theta_init;   // input - w
  p->desired.block(0, 0, 6, 1) = x;

  // create points along the desired line path
  dx = (p->pos_final(0) - p->pos_init(0)) / (double) (p->nb_steps - 1);
  for (int i = 0; i < (p->nb_steps - 2); i++) {
    x = p->desired.block(0, i, 6, 1);

    x(0) += dx;             // state - x
    x(1) = p->vel_init(0);  // state - vx
    x(2) = m * x(0) + c;    // state - z
    x(3) = p->vel_init(1);  // state - vz
    x(4) = p->thrust_init;  // input - az
    x(5) = p->theta_init;   // input - w

    p->desired.block(0, i + 1, 6, 1) = x;
  }

  // push final x
  x(0) = p->pos_final(0);   // state - x
  x(1) = p->vel_final(0);   // state - vx
  x(2) = p->pos_final(1);   // state - z
  x(3) = p->vel_final(1);   // state - vz
  x(4) = p->thrust_final;   // input - az
  x(5) = p->theta_final;    // input - w
  p->desired.block(0, p->nb_steps - 1, 6, 1) = x;

  return 0;
}

int GAProblem::evaluateIndividual(GABitString &bs, struct problem_data *data) {
  int m;
  double cost;
  MatX X;
  VecX g;
  VecX x_opt, x_des;
  VecX z_opt, z_des;
  VecX u1_opt, u2_opt;

  // setup
  cost = 0.0;
  m = data->nb_states + data->nb_inputs;
  load_matrix(bs.chromosome, m, data->nb_steps, X);
  g = 9.81 * MatX::Ones(data->nb_steps, 1);

  // position error cost
  x_opt = X.row(0);
  x_des = data->desired.row(0);
  cost -= data->cost_weights[0] * (x_opt - x_des).squaredNorm();

  z_opt = X.row(2);
  z_des = data->desired.row(2);
  cost -= data->cost_weights[1] * (z_opt - z_des).squaredNorm();

  for (int i = 1; i < bs.nb_time_steps; i++) {
    cost -= pow(fabs(x_opt(i) - x_opt(i - 1)), 4);
    cost -= pow(fabs(z_opt(i) - z_opt(i - 1)), 4);
  }

  // control input cost
  u1_opt = X.row(4);
  u2_opt = X.row(5);
  cost -= data->cost_weights[2] * (u1_opt - g).squaredNorm();
  cost -= data->cost_weights[3] * u2_opt.squaredNorm();

  // set bit string score
  bs.score = cost;

  return 0;
}

int GAProblem::evaluatePopulation(GAPopulation &p, struct problem_data *data) {
  // evaluate individuals
  for (int i = 0; i < p.individuals.size(); i++) {
    this->evaluateIndividual(p.individuals[i], data);
  }

  return 0;
}

int GAProblem::pointCrossover(GABitString &b1, GABitString &b2) {
  int xpoint;
  std::vector<double> chromosome_copy;

  // pre-check
  if (this->configured == false) {
    return -1;
  } else if (this->crossover_probability < randf(0.0, 1.0)) {
    return 0;
  }

  // setup
  xpoint = b1.chromosome.size() / 2;
  chromosome_copy = b1.chromosome;

  // point crossover
  for (int i = 0; i < xpoint; i++) {
    b1.chromosome[i] = b2.chromosome[i];
  }

  for (int i = 0; i < xpoint; i++) {
    b2.chromosome[i] = chromosome_copy[i];
  }

  return 0;
}

int GAProblem::pointMutation(GABitString &bs) {
  // pre-check
  if (this->configured == false) {
    return -1;
  }

  // point mutation
  for (int i = 0; i < (bs.chromosome.size() / 6); i++) {
    // x
    if (this->mutation_probability > randf(0.0, 1.0)) {
      bs.chromosome[i * 6 + 0] = randf(-2.0, 5.0);
    }

    // vx
    if (this->mutation_probability > randf(0.0, 1.0)) {
      bs.chromosome[i * 6 + 1] = randf(-5.0, 5.0);
    }

    // z
    if (this->mutation_probability > randf(0.0, 1.0)) {
      bs.chromosome[i * 6 + 2] = randf(0.0, 10.0);
    }

    // vz
    if (this->mutation_probability > randf(0.0, 1.0)) {
      bs.chromosome[i * 6 + 3] = randf(-5.0, 5.0);
    }

    // az
    if (this->mutation_probability > randf(0.0, 1.0)) {
      bs.chromosome[i * 6 + 4] = randf(-10.0, 10.0);
    }

    // theta
    if (this->mutation_probability > randf(0.0, 1.0)) {
      bs.chromosome[i * 6 + 5] = randf(deg2rad(-90.0), deg2rad(90.0));
    }
  }

  return 0;
}

int GAProblem::tournamentSelection(GAPopulation &p) {
  GABitString bs;
  GABitString best;
  std::vector<GABitString> tournament;
  std::vector<GABitString> selection;

  // pre-check
  if (this->configured == false) {
    return -1;
  }

  // tournament selection
  for (int i = 0; i < p.individuals.size(); i++) {
    // create tournament
    for (int j = 0; j < this->tournament_size; j++) {
      bs = p.individuals[randi(0, p.individuals.size() - 1)];
      tournament.push_back(bs);
    }

    // find best in tournament
    best = tournament[0];
    for (int j = 0; j < this->tournament_size - 1; j++) {
      if (tournament[j].score > best.score) {
        best = tournament[j];
      }
    }

    // add to selection and clear tournament
    selection.push_back(best);
    tournament.clear();
  }

  // replace population with selected individuals
  p.individuals = selection;

  return 0;
}

void GAProblem::findBest(GAPopulation &p, GABitString &best) {
  // pre-load best
  best = p.individuals[0];

  // find best
  for (int i = 1; i < p.individuals.size(); i++) {
    if (p.individuals[i].score > best.score) {
      best = p.individuals[i];
    }
  }
}

int GAProblem::optimize(GAPopulation &p, struct problem_data *data) {
  GABitString best;

  // pre-check
  if (this->configured == false) {
    return -1;
  }

  // optimize
  for (int i = 0; i < this->max_generations; i++) {
    this->evaluatePopulation(p, data);
    this->tournamentSelection(p);

    for (int j = 0; j < (p.individuals.size() / 2); j++) {
      this->pointCrossover(p.individuals[j * 2], p.individuals[j * 2 + 1]);
      this->pointMutation(p.individuals[j * 2]);
      this->pointMutation(p.individuals[j * 2 + 1]);
    }

    this->findBest(p, best);
    std::cout << "gen: " << i << " ";
    std::cout << "score: " << best.score << std::endl;
  }

  return 0;
}

}  // end of awesomo namespace