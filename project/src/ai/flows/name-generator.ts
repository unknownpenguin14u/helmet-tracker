'use server';
/**
 * @fileOverview An AI flow for generating creative names for a company.
 *
 * - generateNames - A function that returns a list of company name ideas.
 * - NameSuggestion - The type for a single name suggestion, including a justification.
 */

import { ai } from '@/ai/genkit';
import { z } from 'genkit';

const NameSuggestionSchema = z.object({
  name: z.string().describe('A creative and memorable company name.'),
  justification: z.string().describe('A brief, one-sentence justification for why the name is a good fit.'),
});

const NameOutputSchema = z.object({
  names: z.array(NameSuggestionSchema),
});

export type NameSuggestion = z.infer<typeof NameSuggestionSchema>;

export async function generateNames(): Promise<NameSuggestion[]> {
  const result = await nameGeneratorFlow();
  return result.names;
}

const nameGeneratorPrompt = ai.definePrompt({
  name: 'nameGeneratorPrompt',
  output: { schema: NameOutputSchema },
  system: `You are a branding expert specializing in naming technology companies.
Generate 5 creative and memorable company names for a business that designs and sells intelligent environment control systems.
The names should be professional, under 40 characters, and suitable for a home automation brand.
Provide a brief (one-sentence) justification for each name.`,
});

const nameGeneratorFlow = ai.defineFlow(
  {
    name: 'nameGeneratorFlow',
    outputSchema: NameOutputSchema,
  },
  async () => {
    const { output } = await nameGeneratorPrompt();
    return output!;
  }
);
