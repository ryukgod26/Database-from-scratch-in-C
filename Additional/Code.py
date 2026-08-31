# Adventure Game

while True:
    name = input("Enter your name: ")
    print("Welcome " + name + "!")
    print("This is an adventure game. You have to survive to win.")

    ready = input("Are you ready? (y/n): ").lower()
    if ready == 'y':
        print("Let's go!")

        print("You are exploring the Himalayas. Suddenly, you slip and fall into a fast-moving river. You're drowning!")
        print("You see a floating log. What will you do?")
        c1 = input("Will you continue to swim in the stream or climb the log? (continue = c / climb = l): ").lower()

        if c1 == 'c':
            print("You swim and spot a boat ahead.")
            c2 = input("Will you continue swimming (s) or climb on the boat (c)? ").lower()

            if c2 == 's':
                print("You get tired and drown. Game over.")
                quit()

            elif c2 == 'c':
                print("You climb into the boat. It's empty.")
                c3 = input("Will you try to start the boat (s) or wait for help (w)? ").lower()

                if c3 == 's':
                    print("You find the engine key and manage to start it. You navigate downstream safely.")
                    c4 = input("You reach a fork in the river. Left goes into a dark cave, right goes into a jungle. (left = l / right = r): ").lower()

                    if c4 == 'l':
                        print("Inside the cave, you find ancient carvings and a safe shelter. You survive! You win!")
                        quit()

                    elif c4 == 'r':
                        print("You are attacked by wild animals in the jungle. Game over.")
                        quit()

                    else:
                        print("Invalid choice. Let's go back to the fork.")
                        continue

                elif c3 == 'w':
                    print("You wait... and wait... until night falls. You freeze in the cold. Game over.")
                    quit()

                else:
                    print("Invalid input. Try again.")
                    continue

            else:
                print("Invalid input. Try again.")
                continue

        elif c1 == 'l':
            print("You climb the log, but a snake hidden underneath bites you!")
            c5 = input("Stay on the log (c) or jump into the water and swim to the bank (h)? ").lower()

            if c5 == 'c':
                print("A fisherman nearby sees you struggling and helps you.")
                c6 = input("Will you go with the fisherman to his camp (g) or stay by the river and rest (r)? ").lower()

                if c6 == 'g':
                    print("You reach a warm camp, eat food, and survive the night. You win!")
                    quit()

                elif c6 == 'r':
                    print("You are attacked by a bear while resting. Game over.")
                    quit()

                else:
                    print("Invalid input. Try again.")
                    continue

            elif c5 == 'h':
                print("You manage to swim to shore, but your leg is hurt.")
                c7 = input("Will you shout for help (s) or crawl through the forest (c)? ").lower()

                if c7 == 's':
                    print("Your shout attracts rescuers nearby. You are saved. You win!")
                    quit()

                elif c7 == 'c':
                    print("You are lost in the forest and don’t survive the night. Game over.")
                    quit()

                else:
                    print("Invalid input. Try again.")
                    continue

            else:
                print("Invalid input. Try again.")
                continue

        else:
            print("Invalid input. Please choose either 'c' or 'l'.")
            continue

    elif ready == 'n':
        print("Thanks for playing!")
        break

    else:
        print("Please enter 'y' or 'n'.")

        