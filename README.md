
---

# Ember Design Overview

## What is Ember?

**Ember** is a statically typed, compiled programming language with modern syntax and strong type inference. It is designed to be expressive, efficient, and fast, with a strong emphasis on developer ergonomics.

The Ember compiler is initially written in **C** and uses **LLVM** as a backend to generate highly optimized machine code.

---

## Key Goals

### Performance

* Minimal runtime overhead
* Strong type inference for optimal memory and instruction layout

### Safety

* Ownership / Borrowing system
* Compiler-enforced checks

### Freedom

* Disable checks with `@ember off`
* Allows:

  * Raw memory access
  * Unsafe concurrency
  * Undefined behavior

### Modern Ergonomics

* Clear type inference (`auto`)
* Flexible multithreading with `@ember parallel`
* High-level abstractions

---

## Memory Management

### Safe Mode (default)

* Ownership and borrowing enforced
* Automatic checks for common issues

### Zig Mode (`@ember off`)

* All runtime checks disabled
* Programmer handles memory and safety
* Maximum performance with minimal overhead

---

## Multithreading

Ember supports **both automatic and explicit multithreading**.

* **Automatic multithreading** via `@ember parallel`
* **Explicit multithreading** with full manual control
* Both approaches can be mixed freely

---

## Libraries

* Strong standard library
* Built-in support for:

  * IO
  * Math
  * Graphics
  * And more

---

## Why Build Ember?

> AI can’t be better at a programming language that I made.
> In addition, it seems feasible, and I need a project for **Flavortown**.

---

## Example Code (Safe Mode)

```ember
// ==========================================================
// Neural Network Example in Ember
// ==========================================================

// Allow parallel execution for training loops
@ember parallel  

// Random number generator (simple)
fn randFloat() -> float {
    return float(rand() % 1000) / 1000.0
}

// Activation function
fn sigmoid(float x) -> float {
    return 1.0 / (1.0 + exp(-x))
}

fn sigmoidDerivative(float x) -> float {
    return x * (1.0 - x)
}

// ==========================================================
// Layer object
// ==========================================================

object Layer {
    int inputSize
    int outputSize
    auto weights     // 2D array: outputSize x inputSize
    auto biases      // 1D array: outputSize

    fn init(int inSize, int outSize) -> Layer layer {
        layer.inputSize = inSize
        layer.outputSize = outSize

        layer.weights = new float[outSize][inSize]
        layer.biases = new float[outSize]

        // Randomly initialize weights and biases
        for i in 0..outSize {
            for j in 0..inSize {
                layer.weights[i][j] = randFloat() - 0.5
            }
            layer.biases[i] = randFloat() - 0.5
        }
    }

    fn forward(auto input) -> float output[] {
        auto output = new float[outputSize]

        for i in 0..outputSize {
            float sum = 0.0
            for j in 0..inputSize {
                sum += input[j] * weights[i][j]
            }
            sum += biases[i]
            output[i] = sigmoid(sum)
        }
        return output
    }
}
```

---

## Neural Network (Safe Mode)

```ember
object NeuralNetwork {
    Layer hidden
    Layer output
    float learningRate

    fn init(int inputSize, int hiddenSize, int outputSize, float lr) -> NeuralNetwork nn {
        nn.hidden = Layer.init(inputSize, hiddenSize)
        nn.output = Layer.init(hiddenSize, outputSize)
        nn.learningRate = lr
    }

    fn predict(auto input) -> float[] {
        auto hiddenOutput = hidden.forward(input)
        auto finalOutput = output.forward(hiddenOutput)
        return finalOutput
    }

    fn train(auto inputs, auto targets, int epochs) {
        for epoch in 0..epochs {

            // Parallel training per sample
            for index, input in inputs {
                auto target = targets[index]

                // Forward pass
                auto hiddenOutput = hidden.forward(input)
                auto finalOutput = output.forward(hiddenOutput)

                // Compute output error
                auto outputErrors = new float[output.outputSize]
                for i in 0..output.outputSize {
                    outputErrors[i] = target[i] - finalOutput[i]
                }

                // Backpropagation - output layer
                auto outputGrad = new float[output.outputSize]
                for i in 0..output.outputSize {
                    outputGrad[i] = outputErrors[i] * sigmoidDerivative(finalOutput[i])
                    for j in 0..hidden.outputSize {
                        output.weights[i][j] += learningRate * outputGrad[i] * hiddenOutput[j]
                    }
                    output.biases[i] += learningRate * outputGrad[i]
                }

                // Backpropagation - hidden layer
                auto hiddenErrors = new float[hidden.outputSize]
                for i in 0..hidden.outputSize {
                    float errorSum = 0.0
                    for j in 0..output.outputSize {
                        errorSum += outputGrad[j] * output.weights[j][i]
                    }
                    hiddenErrors[i] = errorSum
                }

                auto hiddenGrad = new float[hidden.outputSize]
                for i in 0..hidden.outputSize {
                    hiddenGrad[i] = hiddenErrors[i] * sigmoidDerivative(hiddenOutput[i])
                    for j in 0..hidden.inputSize {
                        hidden.weights[i][j] += learningRate * hiddenGrad[i] * input[j]
                    }
                    hidden.biases[i] += learningRate * hiddenGrad[i]
                }
            }
        }
    }
}
```

---

## Unsafe Mode Example (`@ember off`)

> **Warning:** This mode disables all safety checks.
> You are responsible for memory, correctness, and thread safety.

```ember
@ember off  

object Layer {
    int inputSize
    int outputSize
    auto* weights     // raw pointer to 2D array
    auto* biases      // raw pointer to 1D array

    fn init(int inSize, int outSize) -> Layer layer {
        layer.inputSize = inSize
        layer.outputSize = outSize

        layer.weights = alloc<float>(outSize * inSize)
        layer.biases = alloc<float>(outSize)

        for i in 0..outSize {
            for j in 0..inSize {
                layer.weights[i * inSize + j] = randFloat() - 0.5
            }
            layer.biases[i] = randFloat() - 0.5
        }
    }

    fn forward(auto* input) -> float* {
        auto* output = alloc<float>(outputSize)

        for i in 0..outputSize {
            float sum = 0.0
            for j in 0..inputSize {
                sum += input[j] * weights[i * inputSize + j]
            }
            sum += biases[i]
            output[i] = sigmoid(sum)
        }
        return output
    }
}
```

---

## Summary

Ember is designed to sit at the intersection of:

* **Rust-level safety**
* **Zig-level control**
* **Modern, expressive syntax**
* **High-performance compiled execution**

With optional guard rails, Ember lets developers choose **how safe** or **how fast** they want to be per file, per function, or per project.

